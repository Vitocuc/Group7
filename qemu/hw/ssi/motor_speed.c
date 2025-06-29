#include "qemu/osdep.h"
#include "hw/ssi/ssi.h"
#include "qom/object.h"
#include "hw/ssi/motor_speed.h" // Includi il nuovo file di intestazione

// Funzione helper per generare un valore di velocità casuale
static uint16_t generate_random_speed(void) {
    // Genera una velocità casuale tra 0 e 5000 RPM (giri al minuto)
    return (rand() % 5001);
}

/**
 * @brief Callback di trasferimento SPI.
 *
 * Questa funzione viene chiamata ogni volta che il master invia un frame SPI
 * al nostro dispositivo. La logica interpreta il valore ricevuto come un comando.
 *
 * @param dev Puntatore al dispositivo periferico SSI.
 * @param val Valore a 32 bit ricevuto dal master.
 * @return Valore a 32 bit da inviare al master.
 */
static uint32_t motor_speed_sensor_transfer(SSIPeripheral *dev, uint32_t val) {
    MotorSpeedSensor *sensor = MOTOR_SPEED_SENSOR(dev);

    /* Interpreta il valore ricevuto come un comando */
    switch (val) {
        case CMD_GET_SPEED:
            /* Se il master richiede la velocità... */
            sensor->last_speed = generate_random_speed();
            /* Prepara il valore da restituire */
            sensor->transfer_value = sensor->last_speed;
            break;

        default:
            /* Se il comando non è riconosciuto, non fare nulla.
             * Il dispositivo restituirà l'ultimo valore preparato. */
            break;
    }

    return sensor->transfer_value;
}

/**
 * @brief Funzione di inizializzazione del dispositivo (realize).
 *
 * Viene chiamata da QEMU quando il dispositivo viene creato.
 * Inizializza le variabili di stato.
 */
static void motor_speed_sensor_realize(SSIPeripheral *dev, Error **errp) {
    MotorSpeedSensor *sensor = MOTOR_SPEED_SENSOR(dev);

    /* Inizializza la velocità a 0 */
    sensor->last_speed = 0;
    sensor->transfer_value = 0;

    /* Inizializza il seme per la generazione di numeri casuali */
    srand(time(NULL));
}

/**
 * @brief Inizializzazione della classe del dispositivo.
 *
 * Associa le funzioni di callback (transfer, realize) alla classe del dispositivo
 * e imposta altre proprietà di base.
 */
static void motor_speed_sensor_class_init(ObjectClass *klass, void *data) {
    SSIPeripheralClass *ssc = SSI_PERIPHERAL_CLASS(klass);

    /* Associa le nostre funzioni di callback */
    ssc->transfer = motor_speed_sensor_transfer;
    ssc->realize = motor_speed_sensor_realize;

    /* Questo dispositivo non usa una linea Chip Select dedicata */
    ssc->cs_polarity = SSI_CS_NONE;
}

/* Informazioni sul tipo per il sistema di oggetti di QEMU */
static const TypeInfo motor_speed_sensor_type_info = {
    .name          = TYPE_MOTOR_SPEED_SENSOR,
    .parent        = TYPE_SSI_PERIPHERAL,
    .instance_size = sizeof(MotorSpeedSensor),
    .class_init    = motor_speed_sensor_class_init,
};

/* Registra il nuovo tipo di dispositivo in QEMU */
static void motor_speed_sensor_register_types(void)
{
    type_register_static(&motor_speed_sensor_type_info);
}

type_init(motor_speed_sensor_register_types)