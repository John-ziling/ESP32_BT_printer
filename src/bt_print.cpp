#include "bt_print.h"

#define ROW_BYTES       HEAD_BYTES      // 一行 48 字节
#define IDLE_TIMEOUT_MS 2000            // 空闲超过该时间认为本次数据发送完毕
#define POLL_BYTE_BUDGET 512            // 单次 poll 最多处理的字节数，避免长时间占用 loop()/饿死看门狗

static uint8_t  row_buf[ROW_BYTES];
static uint8_t  row_idx = 0;
static bool     printing = false;
static uint32_t last_recv_ms = 0;

static void job_start(void){
    printing = true;
    row_idx = 0;
    printer_begin();
    last_recv_ms = millis();
}

static void job_finish(void){
    // 收尾：把不足一行的高位补 0 打出去
    if(row_idx > 0){
        while(row_idx < ROW_BYTES) row_buf[row_idx++] = 0;
        if(papertest() == 0) printer_print_row(row_buf, HEAT_DEF_TIME_US);  // 缺纸绝不加热
        row_idx = 0;
    }
    motor_run(20 * 4, false, 2000);   // 多送一点纸，避免最后一行压在撕纸口
    motor_release();
    printer_end();
    printing = false;
}

// 异常中止：立即断加热、放线圈，不动电机
static void job_abort(void){
    motor_release();
    printer_end();
    printing = false;
    row_idx = 0;
}

void bt_print_init(void){
    printing = false;
    row_idx = 0;
}

// 目前是"裸字节流 → 攒满 48 字节打一行"的占位实现。
// 喵喵机 App 的帧协议（0x02 | type | seq | len | payload | CRC32 | 0x03）在下一步接入：
// 帧头/握手/ACK 放在这里替换裸流分支，printhead 只需要一行一行喂数据。
void bt_print_poll(void){
    uint16_t budget = POLL_BYTE_BUDGET;

    while(SerialBT.available() && budget--){
        uint8_t b = (uint8_t)SerialBT.read();

        if(!printing){
            if(papertest() != 0) continue;   // 缺纸：丢弃数据，不加热
            job_start();
        }

        row_buf[row_idx++] = b;
        last_recv_ms = millis();

        if(row_idx >= ROW_BYTES){
            if(papertest() != 0){
                Serial.println("打印中缺纸，中止");
                job_abort();
                return;
            }
            printer_print_row(row_buf, HEAT_DEF_TIME_US);
            row_idx = 0;
        }
    }

    // 非阻塞收尾：空闲超时即认为本次发送结束
    if(printing && (millis() - last_recv_ms > IDLE_TIMEOUT_MS)){
        Serial.println("蓝牙打印完成");
        job_finish();
    }
}
