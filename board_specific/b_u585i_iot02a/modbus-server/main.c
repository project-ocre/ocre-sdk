#include "mongoose.h"
#include "ocre_api.h"

//=========================================================================
// Modbus server and register definitions
//=========================================================================

#define MODBUS_TCP_PORT         "1502"
#define MODBUS_TCP_ADDRESS      "tcp://0.0.0.0:" MODBUS_TCP_PORT

#define MODBUS_HEADER_SIZE      7
#define MODBUS_MAX_REGISTERS    64

#define SENSOR_SCAN_INTERVAL_MS 500
#define SENSOR_SCAN_TIMER_ID    1

// LED control
#define REGISTER_LED            0x00
#define REGISTER_LED_MASK_RED   0x01
#define REGISTER_LED_MASK_GREEN 0x02

// Button press count
#define REGISTER_BUTTON         0x01

// Accelerometer data: float32
#define REGISTER_ACCEL_X_L      0x02
#define REGISTER_ACCEL_X_H      0x03
#define REGISTER_ACCEL_Y_L      0x04
#define REGISTER_ACCEL_Y_H      0x05
#define REGISTER_ACCEL_Z_L      0x06
#define REGISTER_ACCEL_Z_H      0x07

// Gyro data: float32
#define REGISTER_GYRO_X_L       0x08
#define REGISTER_GYRO_X_H       0x09
#define REGISTER_GYRO_Y_L       0x0A
#define REGISTER_GYRO_Y_H       0x0B
#define REGISTER_GYRO_Z_L       0x0C
#define REGISTER_GYRO_Z_H       0x0D

// Magnetometer data: float32
#define REGISTER_MAGN_X_L       0x0E
#define REGISTER_MAGN_X_H       0x0F
#define REGISTER_MAGN_Y_L       0x10
#define REGISTER_MAGN_Y_H       0x11
#define REGISTER_MAGN_Z_L       0x12
#define REGISTER_MAGN_Z_H       0x13

// Humidity data: float32
#define REGISTER_HUM_L          0x14
#define REGISTER_HUM_H          0x15
#define REGISTER_TEMP_L         0x16
#define REGISTER_TEMP_H         0x17

// Pressure data: float32
#define REGISTER_PRES_L         0x18
#define REGISTER_PRES_H         0x19

// Light data: float32
#define REGISTER_LIGHT_L        0x20
#define REGISTER_LIGHT_H        0x21

static uint16_t holding_registers[MODBUS_MAX_REGISTERS] = {0};

// Copied from Zephyr
enum sensor_channel {
	/** Acceleration on the X axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_X,
	/** Acceleration on the Y axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Y,
	/** Acceleration on the Z axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Z,
	/** Acceleration on the X, Y and Z axes. */
	SENSOR_CHAN_ACCEL_XYZ,
	/** Angular velocity around the X axis, in radians/s. */
	SENSOR_CHAN_GYRO_X,
	/** Angular velocity around the Y axis, in radians/s. */
	SENSOR_CHAN_GYRO_Y,
	/** Angular velocity around the Z axis, in radians/s. */
	SENSOR_CHAN_GYRO_Z,
	/** Angular velocity around the X, Y and Z axes. */
	SENSOR_CHAN_GYRO_XYZ,
	/** Magnetic field on the X axis, in Gauss. */
	SENSOR_CHAN_MAGN_X,
	/** Magnetic field on the Y axis, in Gauss. */
	SENSOR_CHAN_MAGN_Y,
	/** Magnetic field on the Z axis, in Gauss. */
	SENSOR_CHAN_MAGN_Z,
	/** Magnetic field on the X, Y and Z axes. */
	SENSOR_CHAN_MAGN_XYZ,
	/** Device die temperature in degrees Celsius. */
	SENSOR_CHAN_DIE_TEMP,
	/** Ambient temperature in degrees Celsius. */
	SENSOR_CHAN_AMBIENT_TEMP,
	/** Pressure in kilopascal. */
	SENSOR_CHAN_PRESS,
	/**
	 * Proximity.  Adimensional.  A value of 1 indicates that an
	 * object is close.
	 */
	SENSOR_CHAN_PROX,
	/** Humidity, in percent. */
	SENSOR_CHAN_HUMIDITY,
	/** Illuminance in visible spectrum, in lux. */
	SENSOR_CHAN_LIGHT,
	/** Illuminance in infra-red spectrum, in lux. */
	SENSOR_CHAN_IR,
	/** Illuminance in red spectrum, in lux. */
	SENSOR_CHAN_RED,
	/** Illuminance in green spectrum, in lux. */
	SENSOR_CHAN_GREEN,
	/** Illuminance in blue spectrum, in lux. */
	SENSOR_CHAN_BLUE,
	/** Altitude, in meters */
	SENSOR_CHAN_ALTITUDE,

	/** 1.0 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_1_0,
	/** 2.5 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_2_5,
	/** 10 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_10,
	/** Distance. From sensor to target, in meters */
	SENSOR_CHAN_DISTANCE,

	/** CO2 level, in parts per million (ppm) **/
	SENSOR_CHAN_CO2,
	/** O2 level, in parts per million (ppm) **/
	SENSOR_CHAN_O2,
	/** VOC level, in parts per billion (ppb) **/
	SENSOR_CHAN_VOC,
	/** Gas sensor resistance in ohms. */
	SENSOR_CHAN_GAS_RES,

	/** Voltage, in volts **/
	SENSOR_CHAN_VOLTAGE,

	/** Current Shunt Voltage in milli-volts **/
	SENSOR_CHAN_VSHUNT,

	/** Current, in amps **/
	SENSOR_CHAN_CURRENT,
	/** Power in watts **/
	SENSOR_CHAN_POWER,

	/** Resistance , in Ohm **/
	SENSOR_CHAN_RESISTANCE,

	/** Angular rotation, in degrees */
	SENSOR_CHAN_ROTATION,

	/** Position change on the X axis, in points. */
	SENSOR_CHAN_POS_DX,
	/** Position change on the Y axis, in points. */
	SENSOR_CHAN_POS_DY,
	/** Position change on the Z axis, in points. */
	SENSOR_CHAN_POS_DZ,
	/** Position change on the X, Y and Z axis, in points. */
	SENSOR_CHAN_POS_DXYZ,

	/** Revolutions per minute, in RPM. */
	SENSOR_CHAN_RPM,

	/** Frequency, in Hz. */
	SENSOR_CHAN_FREQUENCY,

	/** Voltage, in volts **/
	SENSOR_CHAN_GAUGE_VOLTAGE,
	/** Average current, in amps **/
	SENSOR_CHAN_GAUGE_AVG_CURRENT,
	/** Standby current, in amps **/
	SENSOR_CHAN_GAUGE_STDBY_CURRENT,
	/** Max load current, in amps **/
	SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT,
	/** Gauge temperature  **/
	SENSOR_CHAN_GAUGE_TEMP,
	/** State of charge measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,
	/** Full Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
	/** Remaining Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
	/** Nominal Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY,
	/** Full Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY,
	/** Average power in mW **/
	SENSOR_CHAN_GAUGE_AVG_POWER,
	/** State of health measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_HEALTH,
	/** Time to empty in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_EMPTY,
	/** Time to full in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_FULL,
	/** Cycle count (total number of charge/discharge cycles) **/
	SENSOR_CHAN_GAUGE_CYCLE_COUNT,
	/** Design voltage of cell in V (max voltage)*/
	SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE,
	/** Desired voltage of cell in V (nominal voltage) */
	SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE,
	/** Desired charging current in mA */
	SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT,
	/** Game Rotation Vector (unit quaternion components X/Y/Z/W) */
	SENSOR_CHAN_GAME_ROTATION_VECTOR,
	/** Gravity Vector (X/Y/Z components in m/s^2) */
	SENSOR_CHAN_GRAVITY_VECTOR,
	/** Gyroscope bias (X/Y/Z components in radians/s) */
	SENSOR_CHAN_GBIAS_XYZ,

	/** All channels. */
	SENSOR_CHAN_ALL,

	/**
	 * Number of all common sensor channels.
	 */
	SENSOR_CHAN_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	SENSOR_CHAN_PRIV_START = SENSOR_CHAN_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	SENSOR_CHAN_MAX = INT16_MAX,
};

#define MAX_CHANNELS_PER_SENSOR 10

typedef struct {
    enum sensor_channel id;
    uint16_t reg;
} channel_map_t;

typedef struct {
    char *name;     // There is a limit in ocre for this length, may want to check it.
    bool active;
    int num_channels;
    channel_map_t map[MAX_CHANNELS_PER_SENSOR];
} sensor_map_t;

sensor_map_t sensors[] = {
    {
        "imu", 
        false,  // Always init to false
        6,
        {
            { SENSOR_CHAN_ACCEL_X, REGISTER_ACCEL_X_L },
            { SENSOR_CHAN_ACCEL_Y, REGISTER_ACCEL_Y_L },
            { SENSOR_CHAN_ACCEL_Z, REGISTER_ACCEL_Z_L },
            { SENSOR_CHAN_GYRO_X,  REGISTER_GYRO_X_L  },
            { SENSOR_CHAN_GYRO_Y,  REGISTER_GYRO_Y_L  },
            { SENSOR_CHAN_GYRO_Z,  REGISTER_GYRO_Z_L  },
        }
    },
    {
        "magnetometer", 
        false,
        3,
        {
            { SENSOR_CHAN_MAGN_X, REGISTER_MAGN_X_L },
            { SENSOR_CHAN_MAGN_Y, REGISTER_MAGN_Y_L },
            { SENSOR_CHAN_MAGN_Z, REGISTER_MAGN_Z_L },
        }
    },
    {
        "humidity",
        false,
        2,
        {
            { SENSOR_CHAN_HUMIDITY,     REGISTER_HUM_L  },
            { SENSOR_CHAN_AMBIENT_TEMP, REGISTER_TEMP_L }
        }
    },
    {
        "pressure",
        false,
        1,
        {
            { SENSOR_CHAN_PRESS, REGISTER_PRES_L },
        }
    },
    {
        "light",
        false,
        1,
        {
            { SENSOR_CHAN_LIGHT, REGISTER_LIGHT_L },
        }
    },
};

// for convenience
int sensor_map_len = sizeof(sensors) / sizeof(sensor_map_t);

// Utility to split float into two 16-bit words (assuming IEEE 754 and little endian)
void float_to_registers(float value, uint16_t *reg_out) {
    union {
        float f;
        uint16_t u16[2];
    } u;
    u.f = value;

    // Store low word first (Modbus register 0 = lower 16 bits)
    reg_out[0] = u.u16[0];
    reg_out[1] = u.u16[1];
}

float register_to_float(uint16_t *reg_in) {
    union {
        float f;
        uint16_t u16[2];
    } u;
    u.u16[0] = reg_in[0]; // Low word
    u.u16[1] = reg_in[1]; // High word
    return u.f;
}

//=======================================================================
// Button configuration and callback
//=======================================================================

// Needed for now since the *by_name API doesn't support GPIO callbacks
#define BUTTON_PORT 2
#define BUTTON_PIN 13

// GPIO callback function for button press
static void button_cb(void)
{
    // For the moment, GPIO callbacks do not support passing the button state and
    // trigger on both edges. Thus, we increment the button press count every other call.
    // Good enough for this demo.
    static bool press_state = 0;
    if (!press_state) {
        holding_registers[REGISTER_BUTTON]++; // Increment button press count
        printf("Press count=%d\n", holding_registers[REGISTER_BUTTON]);
    }
    press_state = !press_state; 
}

int button_init() {
    // Configure button as input
    if (ocre_gpio_configure(BUTTON_PORT, BUTTON_PIN, OCRE_GPIO_DIR_INPUT) != 0)
    {
        printf("Button config failed\n");
        return -1;
    }

    // Register callbacks
    if (ocre_gpio_register_callback(BUTTON_PORT, BUTTON_PIN) != 0)
    {
        printf("Failed to register button callback\n");
        return -1;
    }

    if (ocre_register_gpio_callback(BUTTON_PIN, BUTTON_PORT, button_cb) != 0)
    {
        printf("Failed to register GPIO callback function\n");
        return -1;
    }

    return 0;
}

//=======================================================================
// LED control
//=======================================================================

static void update_leds() {
    printf("Updating LEDs\n");
    // Active high in the registers, active low in the GPIO
    bool led0_state = holding_registers[REGISTER_LED] & REGISTER_LED_MASK_RED;
    bool led1_state = holding_registers[REGISTER_LED] & REGISTER_LED_MASK_GREEN;
    ocre_gpio_set_by_name("led0", led0_state ? OCRE_GPIO_PIN_RESET : OCRE_GPIO_PIN_SET);
    ocre_gpio_set_by_name("led1", led1_state ? OCRE_GPIO_PIN_RESET : OCRE_GPIO_PIN_SET);
}

int led_init() {
    // Configure LEDs as outputs
    if (ocre_gpio_configure_by_name("led0", OCRE_GPIO_DIR_OUTPUT) != 0 ||
        ocre_gpio_configure_by_name("led1", OCRE_GPIO_DIR_OUTPUT) != 0)
    {
        printf("LED config failed\n");
        return -1;
    }

    // Initialize LEDs to OFF
    ocre_gpio_set_by_name("led0", OCRE_GPIO_PIN_SET);
    ocre_gpio_set_by_name("led1", OCRE_GPIO_PIN_SET);
    
    return 0;
}

//=======================================================================
// Sensor configuration
//=======================================================================

void read_sensor(sensor_map_t sensor) {
    for (int ch_idx = 0; ch_idx < sensor.num_channels; ch_idx++) {
        channel_map_t channel = sensor.map[ch_idx];
        // printf("Reading '%s' channel id=%d reg=0x%02X\n", sensor.name, channel.id, channel.reg);
        float val = ocre_sensors_read_by_name(sensor.name, channel.id);
        // printf("%s returned value %0.6f for channel %d\n", sensor.name, val, channel.id);
        float_to_registers(val, &holding_registers[channel.reg]);
    }
}

static void read_sensors() {
    for (int i = 0; i < sensor_map_len; i++) {
        if (sensors[i].active) {
            read_sensor(sensors[i]);
        }
    }
}

int sensor_init() {
    if (ocre_sensors_init() != 0) {
        printf("sensor_init: ocre_sensors_init failure\n");
        return -1;
    }

    int sensor_count = ocre_sensors_discover();
    if (sensor_count <= 0) {
        printf("sensor_init: no sensors discovered\n");
        return -1;
    }

    for (int i = 0; i < sensor_map_len; i++) {
        if (ocre_sensors_open_by_name(sensors[i].name) != 0) {
            printf("sensor_init: could not open sensor '%s'\n", sensors[i].name);
            sensors[i].active = false;
            continue;
        }
        else {
            printf("sensor_init: open sensor '%s' OK\n", sensors[i].name);
            sensors[i].active = true;
        }

        // TODO: check number/type of channels?
    }

    return 0;
}

//=======================================================================
// Modbus functions
//=======================================================================

static void send_exception(struct mg_connection *c, uint8_t unit_id,
                           uint16_t transaction_id, uint8_t function_code, uint8_t exception_code) {
    uint8_t response[9] = {
        (transaction_id >> 8), (transaction_id & 0xFF),
        0x00, 0x00, // Protocol ID
        0x00, 0x03, // Length
        unit_id,
        (function_code | 0x80),
        exception_code
    };
    mg_send(c, response, sizeof(response));
}

static void handle_modbus(struct mg_connection *c, const uint8_t *buf, size_t len) {
    if (len < MODBUS_HEADER_SIZE + 1) return;

    uint16_t transaction_id = (buf[0] << 8) | buf[1];
    uint8_t unit_id = buf[6];
    uint8_t function_code = buf[7];

    switch (function_code) {
        case 0x03: { // Read Holding Registers
            if (len < MODBUS_HEADER_SIZE + 5) break;
            uint16_t start = (buf[8] << 8) | buf[9];
            uint16_t count = (buf[10] << 8) | buf[11];

            if (start + count > MODBUS_MAX_REGISTERS || count > 125) {
                send_exception(c, unit_id, transaction_id, function_code, 0x02); // Illegal data address
                return;
            }

            uint8_t response[260];
            size_t res_len = 0;
            response[res_len++] = buf[0]; response[res_len++] = buf[1]; // Transaction ID
            response[res_len++] = 0x00; response[res_len++] = 0x00;     // Protocol ID
            response[res_len++] = 0x00;
            response[res_len++] = 3 + count * 2;                        // Length
            response[res_len++] = unit_id;
            response[res_len++] = function_code;
            response[res_len++] = count * 2;

            for (uint16_t i = 0; i < count; i++) {
                response[res_len++] = holding_registers[start + i] >> 8;
                response[res_len++] = holding_registers[start + i] & 0xFF;
            }

            mg_send(c, response, res_len);
            break;
        }

        case 0x06: { // Write Single Register
            if (len < MODBUS_HEADER_SIZE + 5) break;
            uint16_t reg = (buf[8] << 8) | buf[9];
            uint16_t value = (buf[10] << 8) | buf[11];

            if (reg != 0x0) {
                // LED register is the only writable register
                send_exception(c, unit_id, transaction_id, function_code, 0x02);
                return;
            }

            if (holding_registers[reg] != value) {
                holding_registers[reg] = value;
                update_leds(); // Update LEDs based on holding registers
                printf("Register %d updated to %d\n", reg, value);
            }
            
            mg_send(c, buf, len); // Echo original request
            break;
        }

        default:
            send_exception(c, unit_id, transaction_id, function_code, 0x01); // Illegal function
            break;
    }
}

static void modbus_slave_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_READ) {
        handle_modbus(c, (const uint8_t *)c->recv.buf, c->recv.len);
        mg_iobuf_del(&c->recv, 0, c->recv.len);  // Clear recv buffer
    }
}

//=======================================================================
// Main
//=======================================================================

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);             // Logs don't show up reliably so disable stdout buffering

    // Initialize GPIO
    if (ocre_gpio_init() != 0)
    {
        printf("GPIO: init failed\n");
        return -1;
    }

    led_init();
    button_init();
    sensor_init();

    // Register timer callback for reading IMU data into modbus registers
    if (ocre_register_timer_callback(SENSOR_SCAN_TIMER_ID, read_sensors) != 0)
    {
        printf("Failed to register timer callback function\n");
        return -1;
    }
    if (ocre_timer_create(SENSOR_SCAN_TIMER_ID) != 0)
    {
        printf("Timer creation failed\n");
        return -1;
    }
    if (ocre_timer_start(SENSOR_SCAN_TIMER_ID, SENSOR_SCAN_INTERVAL_MS, true) != 0)
    {
        printf("Timer start failed\n");
        return -1;
    }
    printf("Sensor read timer started (ID: %d, Interval: %dms)\n", 0, SENSOR_SCAN_INTERVAL_MS);

    // Start Modbus server
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_listen(&mgr, MODBUS_TCP_ADDRESS, modbus_slave_handler, NULL);

    printf("Modbus Listening on %s\n", MODBUS_TCP_ADDRESS);
    
    for (;;) {
        mg_mgr_poll(&mgr, 100);
        // read_sensors();
        ocre_process_events();
    }

    mg_mgr_free(&mgr);
    return 0;
}
