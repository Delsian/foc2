#include "uart_cmd.h"
#include "main.h"
#include "drv/uart_in.h"
#include <stdio.h>

void uart_print_banner(void)
{
    printf("hello\n");
    printf("=== Motor 1 Test Configuration ===\n");
    printf("Motor 1 + Encoder 1 on I2C1 enabled\n");
    printf("Motor 0 disabled for this test\n");
    printf("Commands:\n");
    printf("  + : Increase velocity by 10 RPM\n");
    printf("  - : Decrease velocity by 10 RPM\n");
    printf("  > : Increase amplitude by 5%%\n");
    printf("  < : Decrease amplitude by 5%%\n");
    printf("  p : Toggle position/velocity mode\n");
    printf("  e : Toggle encoder feedback (closed-loop)\n");
    printf("  c : Calibrate encoder offset\n");
    printf("  i : Print info\n");
    printf("  h : Print this help\n");
}

void uart_cmd_process(void)
{
    while (uart_in_available() > 0) {
        uint8_t ch;
        uart_in_getchar(&ch);

        switch (ch) {
            case '+':
                set_event(CMD_VEL_INC);
                break;
            case '-':
                set_event(CMD_VEL_DEC);
                break;
            case '>':
                set_event(CMD_AMP_INC);
                break;
            case '<':
                set_event(CMD_AMP_DEC);
                break;
            case 'p':
            case 'P':
                set_event(CMD_TOGGLE_MODE);
                break;
            case 'e':
            case 'E':
                set_event(CMD_TOGGLE_ENCODER);
                break;
            case 'c':
            case 'C':
                set_event(CMD_CALIBRATE);
                break;
            case 'i':
            case 'I':
                set_event(CMD_INFO);
                break;
            case 'h':
            case 'H':
                set_event(CMD_HELP);
                break;
            default:
                /* Optional: Handle unknown commands or pass through */
                break;
        }
    }
}
