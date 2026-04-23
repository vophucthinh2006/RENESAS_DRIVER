/**
 * @file    main.c
 * @brief   UART + AHT20 I2C Test — EK-RA6M5 (RA6M5 / Cortex-M33)
 *
 * Peripherals:
 *   UART7  (SCI7)  : TX=P613, RX=P614, 115200 baud  → external USB-UART adapter
 *   I2C1   (RIIC1) : SCL=P512, SDA=P511, 100 kHz    → AHT20 sensor
 * (J24-10/J24-9)
 *
 * LED checkpoints (active-HIGH):
 *   LED1 (P006) — blinks 10× fast: TDRE=0 (UART stuck, reflash)
 *                 solid ON:         TDRE=1 (UART functional)
 *   LED2 (P007) — lights after first successful debug_print
 *   LED3 (P008) — toggles every loop iteration (heartbeat)
 */

#include "GPIO.h"
#include "bsp_aht20.h"
#include "debug_print.h"
#include "drv_i2c.h"
#include "drv_uart.h"
#include "rtos_config.h"
#include <stdint.h>

/* ======================================================================
 * LED pin assignments — EK-RA6M5 (active-HIGH)
 * ====================================================================== */
#define LED_PORT GPIO_PORT0
#define LED1_PIN 6U
#define LED2_PIN 7U
#define LED3_PIN 8U

#define LED1_ON() GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_SET)
#define LED2_ON() GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_SET)
#define LED3_ON() GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_SET)
#define LED1_OFF() GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_RESET)
#define LED2_OFF() GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_RESET)
#define LED3_OFF() GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_RESET)

static inline void led3_toggle(void) {
  uint8_t cur = GPIO_Read_Pin(LED_PORT, LED3_PIN);
  GPIO_Write_Pin(LED_PORT, LED3_PIN,
                 (cur != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void led_init(void) {
  GPIO_Config(LED_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  GPIO_Config(LED_PORT, LED2_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  GPIO_Config(LED_PORT, LED3_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  LED1_OFF();
  LED2_OFF();
  LED3_OFF();
}

/* ======================================================================
 * Busy-wait delay — 8 MHz MOCO, -O0: ~4000 iter ≈ 1 ms
 * ====================================================================== */
static void delay_ms_bm(uint32_t ms) {
  volatile uint32_t n = ms * 4000U;
  while (n-- != 0U) {
    __asm volatile("nop");
  }
}

/* ======================================================================
 * i2c_scan — probe every 7-bit address and print responding devices.
 *
 * Interpretation:
 *   No devices found  → I2C bus not working (check pull-up resistors)
 *   Device at 0x38    → AHT20 found, driver should work
 *   Device at other   → wrong pin or different sensor variant
 * ====================================================================== */
static void i2c_scan(I2C_t i2c) {
  uint8_t count = 0U;

  debug_print("I2C scan (0x08-0x77):\r\n");

  for (uint8_t addr = 0x08U; addr <= 0x77U; addr++) {
    I2C_Start(i2c);
    uint8_t ack = I2C_Transmit_Address(i2c, addr, I2C_WRITE);
    I2C_Stop(i2c);

    if (ack) {
      debug_print("  [0x%x] ACK%s\r\n", (unsigned)addr,
                  (addr == AHT20_I2C_ADDR) ? " <- AHT20" : "");
      count++;
    }
  }

  if (count == 0U) {
    debug_print("  No devices found.\r\n");
    debug_print("  Possible causes:\r\n");
    debug_print("  1. Missing pull-up resistors (4.7k to 3.3V on SCL+SDA)\r\n");
    debug_print("  2. Sensor not powered (check VCC/GND)\r\n");
    debug_print("  3. Wrong pin connections\r\n");
  }

  debug_print("\r\n");
}

/* ======================================================================
 * Application Entry Point
 * ====================================================================== */
int main(void) {
  /* --- Checkpoint 0: LED init --------------------------------------- */
  led_init();

  /* --- Checkpoint 1: UART init + TDRE diagnostic ------------------- */
  debug_print_init();

  /* TDRE check: 1=UART running, 0=module stop still active (reflash) */
  uint8_t tdre_ok = (SCI_SSR(OS_DEBUG_UART_CHANNEL) & SSR_TDRE) ? 1U : 0U;

  if (!tdre_ok) {
    /* Blink LED1 10× fast → old binary, MSTPCRB fix not flashed */
    for (uint8_t i = 0U; i < 10U; i++) {
      LED1_ON();
      delay_ms_bm(80U);
      LED1_OFF();
      delay_ms_bm(80U);
    }
  }
  LED1_ON();

  /* --- Checkpoint 2: banner ---------------------------------------- */
  debug_print("\r\n=== RA6M5 UART + AHT20 Test ===\r\n");
  debug_print("UART  : SCI7 P613/P614 @ %u baud\r\n",
              (unsigned)OS_DEBUG_UART_BAUDRATE);
  debug_print("I2C   : RIIC1 P512(SCL)/P511(SDA) @ 100 kHz\r\n");
  debug_print("TDRE  : %s\r\n", tdre_ok ? "OK" : "FAIL (check MSTPCRB)");
  debug_print("================================\r\n\r\n");
  LED2_ON();

  /* --- I2C init + bus scan ----------------------------------------- */
  I2C_Init(I2C1, 8U, I2C_SPEED_STANDARD);
  i2c_scan(I2C1); /* scan before AHT20_Init to see raw bus state */

  /* --- AHT20 init -------------------------------------------------- */
  AHT20_Init(I2C1);

  /* --- Main loop: read AHT20 every 2 s ----------------------------- */
  uint32_t tick = 0U;
  for (;;) {
    AHT20_Data_t data;
    AHT20_Status_t st = AHT20_Read(I2C1, &data);

    if (st == AHT20_OK) {
      /* Scale to 1 decimal place without %f */
      int32_t t10 = (int32_t)(data.temperature_c * 10.0f);
      int32_t rh10 = (int32_t)(data.humidity_pct * 10.0f);

      debug_print("[%u] T=%d.%u C  RH=%d.%u%%\r\n", (unsigned)tick,
                  (int)(t10 / 10), (unsigned)((uint32_t)t10 % 10U),
                  (int)(rh10 / 10), (unsigned)((uint32_t)rh10 % 10U));
    } else {
      debug_print("[%u] AHT20 err=%u  ", (unsigned)tick, (unsigned)st);
      debug_print("(1=NACK 2=BUSY 3=TIMEOUT)\r\n");
    }

    tick++;
    led3_toggle();
    delay_ms_bm(100U);
  }

  return 0;
}
