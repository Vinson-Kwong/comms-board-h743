#include "MainTask.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "main.h"
#include "tl_log.h"
#include "fdcan.h"
#include "cybergear.h"
#include "UART.h"
#include "MTI200.h"
#include "I2C.h"
#include "MPU9250.h"

extern FDCAN_HandleTypeDef hfdcan1;
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

UART *uartLog = NULL;

extern "C" {

// 串口输出重定向
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    if (uartLog) {
        uartLog->Write((uint8_t *)&ch, 1);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    }
    return ch;
}

// 串口输入重定向
#ifdef __GNUC__
int __io_getchar(void)
#else
int fgetc(FILE *f)
#endif /* __GNUC__ */
{
    uint8_t ch = 0;
    if (uartLog) {
        ch = uartLog->ReadBlocking();
    } else {
        HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    }
    return ch;
}

} // extern "C"

void MainTask(void const *pvParameters)
{
    int ret = 0;

    I2C *i2c = new I2C(&hi2c1, 0xD0);
    if (i2c == NULL) {
        TL_LOG_E("I2C object creation failed!");
        return;
    }

    MPU9250 *mpu9250 = new MPU9250(i2c);
    if (mpu9250 == NULL) {
        TL_LOG_E("MPU9250 object creation failed!");
    }

    if (mpu9250) {
        mpu9250->Configure(MPU9250::ACCEL_RANGE_4G, MPU9250::GYRO_RANGE_2000DPS);
        // be lower than half the sample rate to avoid aliasing problems
        // however best results are seen with 250 Hz gyro bandwidth, even though we sample at 200 Hz
        mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_92HZ, MPU9250::DLPF_BANDWIDTH_250HZ);
        // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_OFF, MPU9250::DLPF_BANDWIDTH_OFF);
        // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_184HZ, MPU9250::DLPF_BANDWIDTH_184HZ);
        // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_41HZ, MPU9250::DLPF_BANDWIDTH_41HZ);
        // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_20HZ, MPU9250::DLPF_BANDWIDTH_20HZ);
        // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_OFF, MPU9250::DLPF_BANDWIDTH_184HZ);

        mpu9250->ConfigureInterrupt(GPIOE, GPIO_PIN_3);
    }

    // init uart for log
    uartLog = new UART(&huart2, 115200, UART::UART_OP_MODE_INTERRUPT);
    if (uartLog == NULL) {
        TL_LOG_E("UART object creation failed!");
        return;
    }
    uartLog->RegisterRXcallback(NULL, uartLog, 0);

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

    Cybergear *motor2 = new Cybergear(fdcan, MOTOR_CAN_ID - 1);
    if (motor1 == NULL) {
        TL_LOG_E("motor2(0x%x) creation failed!", MOTOR_CAN_ID - 1);
    }

    Cybergear *motor3 = new Cybergear(fdcan, MOTOR_CAN_ID - 2);
    if (motor1 == NULL) {
        TL_LOG_E("motor3(0x%x) creation failed!", MOTOR_CAN_ID - 2);
    }

    // init uart for xsens IMU (MTi630)
    // UART *uart = new UART(&huart2);
    // MTI200 *mti = new MTI200(uart);
    // if (mti == NULL) {
    //     TL_LOG_E("MTI200 object creation failed!");
    // }
    // mti->Configure(200); // 200Hz sample rate

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
            TL_LOG_E("motor2(0x%x) probe failed!", MOTOR_CAN_ID - 1);
        } else {
            motor2->Enable();
            osDelay(1000);
            motor2->Disable();
        }
    }

    if (motor3) {
        ret = motor3->Probe();
        if (ret != 0) {
            TL_LOG_E("motor3(0x%x) probe failed!", MOTOR_CAN_ID - 2);
        } else {
            motor3->Enable();
            osDelay(1000);
            motor3->Disable();
        }
    }

    return;
}
