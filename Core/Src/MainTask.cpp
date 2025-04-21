#include "MainTask.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "main.h"
#include "tl_log.h"
#include "fdcan.h"
#include "cybergear.h"
#include "UART.h"
#include "MTI200.h"

extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef huart2;

void MainTask(void const *pvParameters)
{
    int ret = 0;

    FDCAN *fdcan = new FDCAN(&hfdcan1);
    if (fdcan == NULL) {
        TL_LOG_E("FDCAN object creation failed!");
        return;
    }

    fdcan->ConfigFilter(FDCAN_EXTENDED_ID, 0);
    fdcan->Start();

    Cybergear *motor1 = new Cybergear(fdcan, MOTOR_CAN_ID);
    if (motor1 == NULL) {
        TL_LOG_E("motor1(0x%x) creation failed!", MOTOR_CAN_ID);
    }

    Cybergear *motor2 = new Cybergear(fdcan, MOTOR_CAN_ID-1);
    if (motor1 == NULL) {
        TL_LOG_E("motor2(0x%x) creation failed!", MOTOR_CAN_ID-1);
    }

    Cybergear *motor3 = new Cybergear(fdcan, MOTOR_CAN_ID-2);
    if (motor1 == NULL) {
        TL_LOG_E("motor3(0x%x) creation failed!", MOTOR_CAN_ID-2);
    }

    // init uart for xsens IMU (MTi630)
    UART *uart = new UART(&huart2);
    MTI200 *mti = new MTI200(uart);
    if (mti == NULL) {
        TL_LOG_E("MTI200 object creation failed!");
    }
    mti->Configure(200); // 200Hz sample rate

    // test motors
    if (motor1) {
        ret = motor1->Probe();
        if (ret != 0) {
            TL_LOG_E("motor1(0x%x) probe failed!", MOTOR_CAN_ID);
        } else {
            motor1->Enable();
            osDelay(1000);
            motor1->Disable();
        }
    }

    if (motor2) {
        ret = motor2->Probe();
        if (ret != 0) {
            TL_LOG_E("motor2(0x%x) probe failed!", MOTOR_CAN_ID-1);
        } else {
            motor2->Enable();
            osDelay(1000);
            motor2->Disable();
        }
    }

    if (motor3) {
        ret = motor3->Probe();
        if (ret != 0) {
            TL_LOG_E("motor3(0x%x) probe failed!", MOTOR_CAN_ID-2);
        } else {
            motor3->Enable();
            osDelay(1000);
            motor3->Disable();
        }  
    }

    if (mti) delete mti;
    if (motor1) delete motor1;
    if (motor2) delete motor2;
    if (motor3) delete motor3;
    if (fdcan) delete fdcan;
    if (uart) delete uart;

    return;
}