#include "canopen.h"
#include "drv/can.h"
#include "main.h"  /* Access to main application variables */
#include <string.h>
#include <stdio.h>

/* Defines */
#define NODE_ID             0x10    /* Default Node ID = 16 */
#define HEARTBEAT_INTERVAL  1000    /* 1000ms */

/* COB-IDs */
#define COB_NMT             0x000
#define COB_SYNC            0x080
#define COB_EMCY            (0x080 + NODE_ID)
#define COB_TIME            0x100
#define COB_SDO_TX          (0x580 + NODE_ID) /* Server -> Client */
#define COB_SDO_RX          (0x600 + NODE_ID) /* Client -> Server */
#define COB_NMT_EC          (0x700 + NODE_ID) /* Heartbeat */

/* NMT Commands */
#define NMT_CMD_START       0x01
#define NMT_CMD_STOP        0x02
#define NMT_CMD_ENTER_PREOP 0x80
#define NMT_CMD_RESET_NODE  0x81
#define NMT_CMD_RESET_COMM  0x82

/* SDO Command Specifiers */
#define SDO_CCS_DOWNLOAD    1
#define SDO_CCS_UPLOAD      2
#define SDO_SCS_UPLOAD      2
#define SDO_SCS_DOWNLOAD    3
#define SDO_ABORT           0x80

/* SDO Abort Codes */
#define ABORT_TOGGLE_BIT    0x05030000
#define ABORT_TIMEOUT       0x05040000
#define ABORT_CMD_SPEC      0x05040001
#define ABORT_UNSUPPORTED   0x06010000
#define ABORT_WRITE_ONLY    0x06010001
#define ABORT_READ_ONLY     0x06010002
#define ABORT_NOT_EXISTS    0x06020000
#define ABORT_PARAM_RANGE   0x06090030

/* External variables from main.c */
extern float target_rpm;
extern float amplitude;
extern bool velocity_mode;
extern float angle;  /* Position mode angle */
extern void set_event(MainCommands cmd); /* Re-trigger control loop if needed */

/* Private variables */
static NMT_State nmt_state = NMT_INITIALIZING;
static uint32_t last_heartbeat = 0;

/* OD Storage */
static uint32_t od_device_type = 0x00000000;
static uint32_t od_error_register = 0x00;
static uint16_t od_producer_heartbeat = 1000;
// static uint32_t od_identity[4] = {0, 0, 0, 0}; /* Vendor, Product, Rev, Serial */

/* Helper for SDO responses */
static void send_sdo_abort(uint16_t index, uint8_t subindex, uint32_t code)
{
    uint8_t data[8];
    data[0] = 0x80;
    data[1] = index & 0xFF;
    data[2] = (index >> 8) & 0xFF;
    data[3] = subindex;
    data[4] = code & 0xFF;
    data[5] = (code >> 8) & 0xFF;
    data[6] = (code >> 16) & 0xFF;
    data[7] = (code >> 24) & 0xFF;
    can_transmit(COB_SDO_TX, data, 8);
}

static void send_sdo_upload_response(uint16_t index, uint8_t subindex, uint8_t *payload, uint8_t len)
{
    uint8_t data[8];
    /* 0x40 | ((4-len) << 2) | 0x02 (expedited) | 0x01 (size indicated)
     * scs = 2 (0x40)
     */
    data[0] = (2 << 5) | ((4 - len) << 2) | 0x03;
    data[1] = index & 0xFF;
    data[2] = (index >> 8) & 0xFF;
    data[3] = subindex;

    memset(&data[4], 0, 4);
    memcpy(&data[4], payload, len);

    can_transmit(COB_SDO_TX, data, 8);
}

static void send_sdo_download_response(uint16_t index, uint8_t subindex)
{
    uint8_t data[8];
    /* scs = 3 (0x60) */
    data[0] = 0x60;
    data[1] = index & 0xFF;
    data[2] = (index >> 8) & 0xFF;
    data[3] = subindex;
    memset(&data[4], 0, 4);

    can_transmit(COB_SDO_TX, data, 8);
}

/* SDO Handler */
static void process_sdo(uint8_t *data, uint8_t len)
{
    uint8_t cmd_spec = data[0] >> 5;
    uint16_t index = data[1] | (data[2] << 8);
    uint8_t subindex = data[3];

    /* Download (Write) to Server */
    if (cmd_spec == SDO_CCS_DOWNLOAD) {
        /* Only supporting expedited transfer (data in payload) */
        if (!(data[0] & 0x02)) {
            send_sdo_abort(index, subindex, ABORT_UNSUPPORTED); /* Segmented not supported */
            return;
        }

        uint8_t data_len = 4 - ((data[0] >> 2) & 0x03);
        (void)data_len; /* Suppress unused variable warning */
        if (!(data[0] & 0x01)) data_len = 4; /* Size not indicated, assume 4 */

        /* Object Dictionary Writes */
        switch (index) {
            case 0x1017: /* Producer Heartbeat Time */
                if (subindex == 0) {
                    od_producer_heartbeat = data[4] | (data[5] << 8);
                    send_sdo_download_response(index, subindex);
                } else {
                    send_sdo_abort(index, subindex, ABORT_NOT_EXISTS);
                }
                break;

            case 0x6040: /* Controlword */
                /* Placeholder for control logic */
                send_sdo_download_response(index, subindex);
                break;

            case 0x6060: /* Modes of Operation */
                if (subindex == 0) {
                    int8_t mode = (int8_t)data[4];
                    if (mode == 1) { /* Position Mode */
                        velocity_mode = false;
                        /* Reset target RPM? */
                    } else if (mode == 3) { /* Velocity Mode */
                        velocity_mode = true;
                    }
                    send_sdo_download_response(index, subindex);
                }
                break;

            case 0x60FF: /* Target Velocity */
                if (subindex == 0) {
                    int32_t val = (int32_t)(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));
                    target_rpm = (float)val;
                    send_sdo_download_response(index, subindex);
                }
                break;

            case 0x2001: /* Amplitude */
                 if (subindex == 0) {
                    uint8_t val = data[4];
                    if (val <= 100) {
                        amplitude = (float)val;
                    }
                    send_sdo_download_response(index, subindex);
                }
                break;

            default:
                send_sdo_abort(index, subindex, ABORT_NOT_EXISTS);
                break;
        }
    }
    /* Upload (Read) from Server */
    else if (cmd_spec == SDO_CCS_UPLOAD) {
        switch (index) {
            case 0x1000: /* Device Type */
                send_sdo_upload_response(index, subindex, (uint8_t*)&od_device_type, 4);
                break;
            case 0x1001: /* Error Register */
                send_sdo_upload_response(index, subindex, (uint8_t*)&od_error_register, 1); // 4? usually U8
                break;
            case 0x1017: /* Producer Heartbeat Time */
                 send_sdo_upload_response(index, subindex, (uint8_t*)&od_producer_heartbeat, 2);
                break;
            case 0x6060: /* Modes of Operation */
                {
                    int8_t mode = velocity_mode ? 3 : 1;
                    send_sdo_upload_response(index, subindex, (uint8_t*)&mode, 1);
                }
                break;
            case 0x60FF: /* Target Velocity */
                {
                    int32_t val = (int32_t)target_rpm;
                    send_sdo_upload_response(index, subindex, (uint8_t*)&val, 4);
                }
                break;
             case 0x2001: /* Amplitude */
                {
                    uint8_t val = (uint8_t)amplitude;
                    send_sdo_upload_response(index, subindex, &val, 1);
                }
                break;
            default:
                send_sdo_abort(index, subindex, ABORT_NOT_EXISTS);
                break;
        }
    }
}

/* NMT Handler */
static void process_nmt(uint8_t *data, uint8_t len)
{
    if (len < 2) return;

    uint8_t cmd = data[0];
    uint8_t nid = data[1];

    if (nid == 0 || nid == NODE_ID) {
        switch (cmd) {
            case NMT_CMD_START:
                nmt_state = NMT_OPERATIONAL;
                // printf("NMT: Start Node\n");
                break;
            case NMT_CMD_STOP:
                nmt_state = NMT_STOPPED;
                // printf("NMT: Stop Node\n");
                break;
            case NMT_CMD_ENTER_PREOP:
                nmt_state = NMT_PRE_OPERATIONAL;
                // printf("NMT: Pre-Operational\n");
                break;
            case NMT_CMD_RESET_NODE:
            case NMT_CMD_RESET_COMM:
                nmt_state = NMT_INITIALIZING;
                // printf("NMT: Reset Node\n");
                canopen_init(); /* Re-init */
                break;
        }
    }
}

void canopen_init(void)
{
    nmt_state = NMT_INITIALIZING;
    /* Perform any other initialization */

    /* Transition to Pre-Operational automatically after Init */
    nmt_state = NMT_PRE_OPERATIONAL;

    /* Send Boot-up message (Heartbeat with state 0) */
     uint8_t data[1] = {0x00};
     can_transmit(COB_NMT_EC, data, 1);
}

void canopen_process(void)
{
    /* Heartbeat Producer */
    if (od_producer_heartbeat > 0) {
        uint32_t now = HAL_GetTick();
        if (now - last_heartbeat >= od_producer_heartbeat) {
            last_heartbeat = now;
            uint8_t data[1];
            data[0] = (uint8_t)nmt_state;
            can_transmit(COB_NMT_EC, data, 1);
        }
    }
}

void canopen_handle_rx(uint32_t id, uint8_t* data, uint8_t len)
{
    if (id == COB_NMT) {
        process_nmt(data, len);
    } else if (id == COB_SDO_RX) {
        process_sdo(data, len);
    }
}

NMT_State canopen_get_state(void)
{
    return nmt_state;
}
