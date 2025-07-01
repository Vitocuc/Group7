/* motor_speed.h */
#ifndef HW_SSI_MOTOR_SPEED_H
#define HW_SSI_MOTOR_SPEED_H

#include "hw/ssi/ssi.h"
#include "qom/object.h"

#define TYPE_MOTOR_SPEED_SENSOR "motor-speed-sensor"

/*
 * CORREZIONE: Ho usato il nome corretto della macro: OBJECT_DECLARE_TYPE
 */
OBJECT_DECLARE_TYPE(MotorSpeedSensor, motor_speed_sensor, MOTOR_SPEED_SENSOR);

/* Comando per richiedere la velocità del motore */
#define CMD_GET_SPEED 0xAA

/**
 * @brief Struttura dello stato del sensore di velocità.
 *
 * Contiene lo stato interno del nostro dispositivo slave.
 */
struct MotorSpeedSensor {
    SSIPeripheral parent_obj;

    /* Ultima velocità generata (in RPM) */
    uint16_t last_speed;
    /* Valore che verrà trasferito al master SPI */
    uint16_t transfer_value;
};

#endif /* HW_SSI_MOTOR_SPEED_H */