#include "mongoose.h"
#include "ocre_api.h"

#define MODBUS_TCP_PORT     "1502"
#define MODBUS_TCP_ADDRESS  "tcp://0.0.0.0:" MODBUS_TCP_PORT

#define MODBUS_HEADER_SIZE   7
#define MODBUS_MAX_REGISTERS 64

static uint16_t holding_registers[MODBUS_MAX_REGISTERS] = {0};

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

            if (reg >= MODBUS_MAX_REGISTERS) {
                send_exception(c, unit_id, transaction_id, function_code, 0x02);
                return;
            }

            holding_registers[reg] = value;
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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);             // Logs don't show up reliably so disable stdout buffering

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    mg_listen(&mgr, MODBUS_TCP_ADDRESS, modbus_slave_handler, NULL);

    printf("Modbus Listening on %s\n", MODBUS_TCP_ADDRESS);
    
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    
    mg_mgr_free(&mgr);
    return 0;
}
