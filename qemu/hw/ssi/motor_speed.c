#include "qemu/osdep.h"
#include "hw/ssi/ssi.h"
#include "qom/object.h"
#include "hw/ssi/motor_speed.h"

// Funzione helper per generare un valore di velocità casuale
static uint16_t generate_random_speed(void) {
    // Genera una velocità casuale tra 0 e 5000 RPM (giri al minuto)
    return (rand() % 255);
}

// Callback di trasferimento SPI
static uint32_t motor_speed_sensor_transfer(SSIPeripheral *dev, uint32_t val) {
    MotorSpeedSensor *sensor = MOTOR_SPEED_SENSOR(dev);

    switch (val) {
        case CMD_GET_SPEED:
            sensor->last_speed = generate_random_speed();
            sensor->transfer_value = sensor->last_speed;
            break;
        default:
            break;
    }
    return sensor->transfer_value;
}

// Funzione di inizializzazione del dispositivo (realize)
static void motor_speed_sensor_realize(SSIPeripheral *dev, Error **errp) {
    MotorSpeedSensor *sensor = MOTOR_SPEED_SENSOR(dev);

    sensor->last_speed = 0;
    sensor->transfer_value = 0;
    srand(time(NULL));
}

/*
 * CORREZIONE: Aggiunto 'const' al secondo argomento 'data' per
 * corrispondere alla firma richiesta da TypeInfo.class_init.
 */
static void motor_speed_sensor_class_init(ObjectClass *klass, const void *data) {
    SSIPeripheralClass *ssc = SSI_PERIPHERAL_CLASS(klass);

    ssc->transfer = motor_speed_sensor_transfer;
    ssc->realize = motor_speed_sensor_realize;
    ssc->cs_polarity = SSI_CS_NONE;
}

// Informazioni sul tipo per il sistema di oggetti di QEMU
static const TypeInfo motor_speed_sensor_type_info = {
    .name          = TYPE_MOTOR_SPEED_SENSOR,
    .parent        = TYPE_SSI_PERIPHERAL,
    .instance_size = sizeof(MotorSpeedSensor),
    .class_init    = motor_speed_sensor_class_init,
};

// Registra il nuovo tipo di dispositivo in QEMU
static void motor_speed_sensor_register_types(void)
{
    type_register_static(&motor_speed_sensor_type_info);
}

type_init(motor_speed_sensor_register_types)