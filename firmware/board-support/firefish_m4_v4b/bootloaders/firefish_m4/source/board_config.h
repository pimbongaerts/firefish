#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#define CRYSTALLESS    1

#define VENDOR_NAME "Pim Bongaerts"
#define PRODUCT_NAME "FireFish"
#define VOLUME_LABEL "FIREBOOT"
#define INDEX_URL "https://github.com/pimbongaerts/firefish"
#define BOARD_ID "FIREFISH_02"

#define USB_VID 0x239A
#define USB_PID 0x007D

#define LED_PIN PIN_PB23

#define BOOT_USART_MODULE                 SERCOM5
#define BOOT_USART_MASK                   APBAMASK
#define BOOT_USART_BUS_CLOCK_INDEX        MCLK_APBDMASK_SERCOM5
#define BOOT_USART_PAD_SETTINGS           UART_RX_PAD1_TX_PAD0
#define BOOT_USART_PAD3                   PINMUX_UNUSED
#define BOOT_USART_PAD2                   PINMUX_UNUSED
#define BOOT_USART_PAD1                   PINMUX_PA22D_SERCOM5_PAD1
#define BOOT_USART_PAD0                   PINMUX_PA23D_SERCOM5_PAD0
#define BOOT_GCLK_ID_CORE                 SERCOM5_GCLK_ID_CORE
#define BOOT_GCLK_ID_SLOW                 SERCOM5_GCLK_ID_SLOW

#endif
