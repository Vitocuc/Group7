/* motor_speed.h */
#ifndef HW_SSI_MOTOR_SPEED_H
#define HW_SSI_MOTOR_SPEED_H

#include "hw/ssi/ssi.h"
#include "qom/object.h"

#define TYPE_MOTOR_SPEED_SENSOR "motor-speed-sensor"
OBJECT_DECLARE_TYPE(MotorSpeedSensor, motor_speed_sensor, MOTOR_SPEED_SENSOR);

/* Command to request the motor speed */
#define CMD_GET_SPEED 0xAA

/* Structure of the speed sensor state */
struct MotorSpeedSensor
{
    SSIPeripheral parent_obj;

    /* Last generated speed (in RPM) */
    uint16_t last_speed;
    /* Value that will be transferred to the SPI master */
    uint16_t transfer_value;
};

#endif /* HW_SSI_MOTOR_SPEED_H */