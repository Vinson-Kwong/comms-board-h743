#include "KugleMainTask.h"
#include "cmsis_os.h"
#include "Priorities.h"
#include "tl_log.h"

/* Include Periphiral drivers */
#include "EEPROM.h"
#include "Encoder.h"
#include "I2C.h"
#include "IO.h"
#include "PWM.h"
#include "Timer.h"
#include "UART.h"
// #include "Watchdog.h"

/* Include Device drivers */
// #include "Battery.h"
#include "ESCON.h"
#include "IMU.h"
#include "LSPC.hpp"
#include "MPU9250.h"
#include "MTI200.h"
// #include "QuadratureKnob.h"

/* Include Module libraries */
#include "LQR.h"
#include "SlidingMode.h"
#include "Debug.h"
#include "COMEKF.h"
#include "MadgwickAHRS.h"
#include "QEKF.h"
#include "VelocityEKF.h"
#include "Parameters.h"
// #include "PowerManagement.h"
// #include "Joystick.h"

/* Include Application-layer libraries */
#include "BalanceController.h"
// #include "Communication.h"
// #include "FrontPanel.h"
// #include "PathFollowingController.h"
// #include "ApplicationTemplate.h"

/* Miscellaneous includes */
#include "MATLABCoderInit.h"
#include <stdlib.h>
#include <vector>
// #include "ProcessorInit.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart10;

void KugleMainTask(void const *pvParameters)
{
  /* Use this task to:
   * - Create objects for each module
   *     (OBS! It is very important that objects created with "new"
   *      only happens within a thread due to the usage of the FreeRTOS managed heap)
   * - Link any modules together if necessary
   * - Create message exchange queues and/or semaphore
   * - (Create) and start threads related to modules
   *
   * Basically anything related to starting the system should happen in this thread and NOT in the main() function !!!
   */

  /* Initialize global parameters */
  Parameters &params = *(new Parameters);

  /* Initialize EEPROM */
  EEPROM *eeprom = new EEPROM;
  eeprom->EnableSection(eeprom->sections.imu_calibration,
                        sizeof(IMU::calibration_t)); // enable IMU calibration section in EEPROM
  eeprom->EnableSection(eeprom->sections.parameters, params.getParameterSizeBytes());
  eeprom->Initialize();
  params.AttachEEPROM(eeprom); // attach EEPROM to load and store parameters into EEPROM

  /* Initialize MATLAB coder globals */
  MATLABCoder_initialize();

  /* Debug boot info */
  printf("MainTask...\r\n");

  /* Initialize front panel periphirals (eg. quadrature knob, LCD and buttons */
  // IO * powerButton = new IO(GPIOB, GPIO_PIN_6, IO::PULL_DOWN); // configure as input
  // IO * resetButton = new IO(GPIOD, GPIO_PIN_15, IO::PULL_DOWN); // configure as input
  IO *calibrateButton = new IO(GPIOD, GPIO_PIN_14, IO::PULL_DOWN); // configure as input

  /* Prepare Xsens IMU always, since it is used for logging and comparison purposes */
  UART *uart = new UART(&huart10, 460800);
  MTI200 *mti200 = new MTI200(uart);
  if (params.estimator.ConfigureXsensIMUatBoot) {
    if (!mti200->Configure(
          2 * params.estimator.SampleRate)) { // configuration failed, so do not use/pass on to balance controller
      delete (mti200);
      mti200 = 0;
    }
  }

  /* Initialize and configure IMU */
  IMU *imu = 0;
  if (params.estimator.UseXsensIMU) {
    if (!mti200)
      ERROR("MTI200 selected but not available!");
    imu = mti200; // use Xsens MTI200 in Balance controller
    imu->AttachEEPROM(eeprom);
  } else {
    // Prepare and configure MPU9250 IMU
    // SPI *spi = new SPI(SPI::PORT_SPI6, MPU9250_Bus::SPI_LOW_FREQUENCY, GPIOG, GPIO_PIN_8);
    // I2C *i2c = new I2C(&hi2c1, 0xD0); // 0xD0 is the I2C address of the MPU9250
    // MPU9250 *mpu9250 = new MPU9250(i2c);
    // if (mpu9250->Configure(MPU9250::ACCEL_RANGE_4G, MPU9250::GYRO_RANGE_2000DPS) != 0)
    //   ERROR("MPU9250 selected but not available!");

    // if (params.estimator.EnableSensorLPFfilters) {
    //   // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_92HZ, MPU9250::DLPF_BANDWIDTH_92HZ); // sensor bandwidths should be
    //   // lower than half the sample rate to avoid aliasing problems
    //   mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_92HZ,
    //                    MPU9250::DLPF_BANDWIDTH_250HZ); // however best results are seen with 250 Hz gyro bandwidth, even
    //                                                    // though we sample at 200 Hz
    // } else {
    //   mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_OFF, MPU9250::DLPF_BANDWIDTH_OFF);
    //   // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_184HZ, MPU9250::DLPF_BANDWIDTH_184HZ);
    //   // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_41HZ, MPU9250::DLPF_BANDWIDTH_41HZ);
    //   // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_20HZ, MPU9250::DLPF_BANDWIDTH_20HZ);
    //   // mpu9250->setFilt(MPU9250::DLPF_BANDWIDTH_OFF, MPU9250::DLPF_BANDWIDTH_184HZ);
    // }
    // mpu9250->ConfigureInterrupt(GPIOE, GPIO_PIN_3);
    // imu = mpu9250; // use MPU9250 in Balance controller
    // imu->AttachEEPROM(eeprom);
  }
  if (!imu->isCalibrated()) {
    imu->SetCalibration(params.sensor.default_accelerometer_bias, params.sensor.default_accelerometer_scale,
                        params.sensor.default_gyroscope_bias, params.sensor.default_calibration_matrix);
  }

  /* Initialize microseconds timer */
  Timer *microsTimer = new Timer(Timer::TIMER6, 1000000); // create a 1 MHz counting timer used for micros() timing

  while (1) {
    osDelay(1000);
  }
}
