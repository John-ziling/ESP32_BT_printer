#include "bt_print.h"

#define ROW_BYTES HEAD_BYTES
#define IDLE_TIMEOUT_MS 2000

static uint8_t row_buf[ROW_BYTES];
static uint8_t  row_idx = 0;
static bool     printing = false;
static uint32_t last_recv_ms = 0;

static void job_start(void){
    printing = true; row_idx = 0; printer_begin(); last_recv_ms = millis();
}
static void job_finish(void){
    if(row_idx > 0){
        while(row_idx < ROW_BYTES) row_buf[row_idx++] = 0;
        if(papertest() == 0) printer_print_row(row_buf, HEAT_DEF_TIME_US);
        row_idx = 0;
    }
    motor_run(20 * 4, false, 2000);
    motor_release();
    printer_end();
    printing = false;
}
static void job_abort(void){
    motor_release(); printer_end(); printing = false; row_idx = 0;
}

void bt_print_init(void){ printing = false; row_idx = 0; }

void bt_print_poll(void){
    while(SerialBT.available()){
        uint8_t b = (uint8_t)SerialBT.read();

        // 空闲时，以 0xA5 开头当作协议帧（目前只支持 PING）
        if(!printing && b == BTP_SOP){
            handle_ping_frame();
            continue;
        }

        // 其余字节按裸流攒行打印
        if(!printing){
            if(papertest() != 0) continue;
            job_start();
        }
        row_buf[row_idx++] = b;
        last_recv_ms = millis();
        if(row_idx >= ROW_BYTES){
            if(papertest() != 0){ Serial.println("打印中缺纸，中止"); job_abort(); return; }
            printer_print_row(row_buf, HEAT_DEF_TIME_US);
            row_idx = 0;
        }
    }
    if(printing && (millis() - last_recv_ms > IDLE_TIMEOUT_MS)){
        Serial.println("蓝牙打印完成");
        job_finish();
    }
}