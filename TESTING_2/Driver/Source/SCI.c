#include "SCI.h"

//CLK
void SCI_Clock_Init(SCI_t peripheral){
    RWP_Unlock_Clock_MSTP();
    LPM_Unlock(peripheral);
    RWP_Lock_Clock_MSTP();
}

//UART
void UART_PinConfig(UART_t UART_)
{
    uint8_t tx_port, tx_pin;
    uint8_t rx_port, rx_pin;
    uint32_t psel;

    switch (UART_)
    {
        case UART0:
            tx_port = 1; tx_pin = 1;
            rx_port = 1; rx_pin = 0;
            psel = 0x04;
            break;

        case UART1:
            tx_port = 7; tx_pin = 9;
            rx_port = 7; rx_pin = 8;
            psel = 0x05;
            break;

        case UART2:
            tx_port = 3; tx_pin = 2;
            rx_port = 3; rx_pin = 1;
            psel = 0x04;
            break;

        case UART3:
            tx_port = 3; tx_pin = 10;
            rx_port = 3; rx_pin = 9;
            psel = 0x05;
            break;

        case UART4:
            tx_port = 2; tx_pin = 5;
            rx_port = 2; rx_pin = 6;
            psel = 0x04;
            break;

        case UART5:
            tx_port = 5; tx_pin = 1;
            rx_port = 5; rx_pin = 2;
            psel = 0x05;
            break;

        case UART6:
            tx_port = 5; tx_pin = 6;
            rx_port = 5; rx_pin = 5;
            psel = 0x04;
            break;

        case UART7:
            tx_port = 4; tx_pin = 1;
            rx_port = 4; rx_pin = 3;
            psel = 0x05;
            break;

        case UART8:
            tx_port = 10; tx_pin = 0; // PORTA = 10
            rx_port = 6;  rx_pin = 7;
            psel = 0x04;
            break;

        case UART9:
            tx_port = 1; tx_pin = 9;
            rx_port = 1; rx_pin = 10;
            psel = 0x05;
            break;

        default:
            return;
    }
    PWPR = 0x00;

    PmnPFS(rx_port, rx_pin) = PmnPFS_PSEL(psel);
    PmnPFS(rx_port, rx_pin) |= PmnPFS_PMR;
    PDR(rx_port) &= ~(1 << rx_pin);

    PmnPFS(tx_port, tx_pin) = PmnPFS_PSEL(psel);
    PmnPFS(tx_port, tx_pin) |= PmnPFS_PMR;
    PDR(tx_port) |= (1 << tx_pin);

    PWPR = 0x80;
}
void UART_Init(UART_t UART_, uint32_t baudrate){
    volatile SCI_t peripheral;
    volatile uint8_t *SMR;
    volatile uint8_t *BRR;
    volatile uint8_t *SCR;
    volatile uint8_t *SSR;

    switch (UART_){
        case UART0:
            peripheral = SCI0;
            SMR = &SCI0_SMR;
            BRR = &SCI0_BRR;
            SCR = &SCI0_SCR;
            SSR = &SCI0_SSR;
            break;
        case UART1:
            peripheral = SCI1;
            SMR = &SCI1_SMR;
            BRR = &SCI1_BRR;
            SCR = &SCI1_SCR;
            SSR = &SCI1_SSR;
            break;
        case UART2:
            peripheral = SCI2;
            SMR = &SCI2_SMR;
            BRR = &SCI2_BRR;
            SCR = &SCI2_SCR;
            SSR = &SCI2_SSR;
            break;
        case UART3:
            peripheral = SCI3;
            SMR = &SCI3_SMR;
            BRR = &SCI3_BRR;
            SCR = &SCI3_SCR;
            SSR = &SCI3_SSR;
            break;
        case UART4:
            peripheral = SCI4;
            SMR = &SCI4_SMR;
            BRR = &SCI4_BRR;
            SCR = &SCI4_SCR;
            SSR = &SCI4_SSR;
            break;
        case UART5:
            peripheral = SCI5;
            SMR = &SCI5_SMR;
            BRR = &SCI5_BRR;
            SCR = &SCI5_SCR;
            SSR = &SCI5_SSR;
            break;
        case UART6:
            peripheral = SCI6;
            SMR = &SCI6_SMR;
            BRR = &SCI6_BRR;
            SCR = &SCI6_SCR;
            SSR = &SCI6_SSR;
            break;
        case UART7:
            peripheral = SCI7;
            SMR = &SCI7_SMR;
            BRR = &SCI7_BRR;
            SCR = &SCI7_SCR;
            SSR = &SCI7_SSR;
            break;
        case UART8:
            peripheral = SCI8;
            SMR = &SCI8_SMR;
            BRR = &SCI8_BRR;
            SCR = &SCI8_SCR;
            SSR = &SCI8_SSR;
            break;
        case UART9:
            peripheral = SCI9;
            SMR = &SCI9_SMR;
            BRR = &SCI9_BRR;
            SCR = &SCI9_SCR;
            SSR = &SCI9_SSR;
            break;
        default: break;
    }
    SCI_Clock_Init(peripheral);
    UART_PinConfig(UART_);
    *SCR = 0x00;
    *SMR = 0x00;
    *BRR = (uint8_t)((PCLKB / (16UL * baudrate)) - 1);
    *SSR = 0x00;
    *SCR = (SCR_TE | SCR_RE);
}
void UART_SendChar(UART_t UART_, char data){
    volatile uint8_t *TDR;
    volatile uint8_t *SSR;

    switch (UART_){
        case UART0:
            TDR = &SCI0_TDR;
            SSR = &SCI0_SSR;
            break;
        case UART1:
            TDR = &SCI1_TDR;
            SSR = &SCI1_SSR;
            break;
        case UART2:
            TDR = &SCI2_TDR;
            SSR = &SCI2_SSR;
            break;
        case UART3:
            TDR = &SCI3_TDR;
            SSR = &SCI3_SSR;
            break;
        case UART4:
            TDR = &SCI4_TDR;
            SSR = &SCI4_SSR;
            break;
        case UART5:
            TDR = &SCI5_TDR;
            SSR = &SCI5_SSR;
            break;
        case UART6:
            TDR = &SCI6_TDR;
            SSR = &SCI6_SSR;
            break;
        case UART7:
            TDR = &SCI7_TDR;
            SSR = &SCI7_SSR;
            break;
        case UART8:
            TDR = &SCI8_TDR;
            SSR = &SCI8_SSR;
            break;
        case UART9:
            TDR = &SCI9_TDR;
            SSR = &SCI9_SSR;
            break;
        default: break;
    }
    while (!(*SSR & SSR_TDRE));
    *TDR = data;
    *SSR &= ~SSR_TDRE;
}
void UART_SendString(UART_t UART_, const char *str){
    while (*str) UART_SendChar(UART_, *str++);
}
char UART_ReceiveChar(UART_t UART_){
    volatile uint8_t *SSR;
    volatile uint8_t *RDR;

    switch (UART_){
        case UART0:
            SSR = &SCI0_SSR;
            RDR = &SCI0_RDR;
            break;
        case UART1:
            SSR = &SCI1_SSR;
            RDR = &SCI1_RDR;
            break;
        case UART2:
            SSR = &SCI2_SSR;
            RDR = &SCI2_RDR;
            break;
        case UART3:
            SSR = &SCI3_SSR;
            RDR = &SCI3_RDR;
            break;
        case UART4:
            SSR = &SCI4_SSR;
            RDR = &SCI4_RDR;
            break;
        case UART5:
            SSR = &SCI5_SSR;
            RDR = &SCI5_RDR;
            break;
        case UART6:
            SSR = &SCI6_SSR;
            RDR = &SCI6_RDR;
            break;
        case UART7:
            SSR = &SCI7_SSR;
            RDR = &SCI7_RDR;
            break;
        case UART8:
            SSR = &SCI8_SSR;
            RDR = &SCI8_RDR;
            break;
        case UART9:
            SSR = &SCI9_SSR;
            RDR = &SCI9_RDR;
            break;
        default: break;
    }
    while (!(*SSR & SSR_RDRF));
    char data = *RDR;
    return data;
}
