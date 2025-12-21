#include "drv/can.h"
#include <stdio.h>

static FDCAN_HandleTypeDef *hcan = NULL;

void can_init(FDCAN_HandleTypeDef *hfdcan)
{
    hcan = hfdcan;

    FDCAN_FilterTypeDef sFilterConfig;

    /* Configure RX filter to accept all standard IDs */
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = 0x000;
    sFilterConfig.FilterID2 = 0x000;

    if (HAL_FDCAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK) {
        printf("CAN Filter config failed\n");
        Error_Handler();
    }

    /* Configure global filter to accept all standard frames, reject extended and remote frames */
    if (HAL_FDCAN_ConfigGlobalFilter(hcan, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_REJECT,
                                     FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) {
        printf("CAN Global filter config failed\n");
        Error_Handler();
    }

    /* Activate notification for RX FIFO 0 new message */
    if (HAL_FDCAN_ActivateNotification(hcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        printf("CAN Notification activation failed\n");
        Error_Handler();
    }

    if (HAL_FDCAN_Start(hcan) != HAL_OK) {
        printf("CAN Start failed\n");
        Error_Handler();
    }
    printf("CAN initialized successfully\n");
}

void can_transmit(uint32_t id, uint8_t *data, uint8_t len)
{
    if (hcan == NULL) {
        return;
    }

    /* Debug: Print FDCAN register values once */
    static uint8_t debug_printed = 0;
    if (!debug_printed) {
        printf("FDCAN NBTP: 0x%08lX\n", hcan->Instance->NBTP);
        printf("FDCAN DBTP: 0x%08lX\n", hcan->Instance->DBTP);
        printf("FDCAN CCCR: 0x%08lX\n", hcan->Instance->CCCR);

        /* Read RCC registers to verify clock configuration */
        printf("RCC PLLCFGR: 0x%08lX\n", RCC->PLLCFGR);
        printf("RCC CCIPR: 0x%08lX\n", RCC->CCIPR);
        printf("RCC CR: 0x%08lX\n", RCC->CR);

        /* Calculate actual FDCAN clock from CCIPR register */
        uint32_t fdcan_clk_sel = (RCC->CCIPR >> 24) & 0x3;
        printf("FDCAN Clock Source: %lu (0=PLLQ, 1=PLL, 2=HSE)\n", fdcan_clk_sel);

        debug_printed = 1;
    }

    FDCAN_TxHeaderTypeDef TxHeader;

    /* Convert length to DLC code */
    uint32_t dlc_code;
    switch (len) {
        case 0: dlc_code = FDCAN_DLC_BYTES_0; break;
        case 1: dlc_code = FDCAN_DLC_BYTES_1; break;
        case 2: dlc_code = FDCAN_DLC_BYTES_2; break;
        case 3: dlc_code = FDCAN_DLC_BYTES_3; break;
        case 4: dlc_code = FDCAN_DLC_BYTES_4; break;
        case 5: dlc_code = FDCAN_DLC_BYTES_5; break;
        case 6: dlc_code = FDCAN_DLC_BYTES_6; break;
        case 7: dlc_code = FDCAN_DLC_BYTES_7; break;
        case 8: dlc_code = FDCAN_DLC_BYTES_8; break;
        default: dlc_code = FDCAN_DLC_BYTES_8; break;
    }

    TxHeader.Identifier = id;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = dlc_code;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    HAL_StatusTypeDef status = HAL_FDCAN_AddMessageToTxFifoQ(hcan, &TxHeader, data);
    if (status != HAL_OK) {
        printf("CAN TX Error: %d\n", status);
    } else {
        printf("CAN TX OK: ID=0x%lX DLC=%d\n", id, len);
    }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0) {
        FDCAN_RxHeaderTypeDef RxHeader;
        uint8_t RxData[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK) {
            /* Print CAN packet information */
            printf("CAN RX: ID=0x%03lX DLC=%lu Data=",
                   RxHeader.Identifier,
                   RxHeader.DataLength >> 16);

            uint8_t dlc = RxHeader.DataLength >> 16;
            for (uint8_t i = 0; i < dlc; i++) {
                printf("%02X ", RxData[i]);
            }
            printf("\n");
        }
    }
}
