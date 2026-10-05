#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "led_strip.h"

extern "C" {
#include "pn532.h"
}

static const char *TAG = "PN532_RGB_DEBUG";

// Configuración de Pines
#define NFC_UART_PORT      UART_NUM_1
#define PIN_NFC_TX         GPIO_NUM_17 // ESP32 TX -> PN532 RXD
#define PIN_NFC_RX         GPIO_NUM_18 // ESP32 RX <- PN532 TXD
#define LED_RGB_GPIO       GPIO_NUM_48 // LED WS2812 integrado en la placa ESP32-S3
#define NFC_BAUD_RATE      115200

static led_strip_handle_t led_strip;

// Función auxiliar para cambiar el color del LED RGB
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue)
{
    if (led_strip) {
        led_strip_set_pixel(led_strip, 0, red, green, blue);
        led_strip_refresh(led_strip);
    }
}

// Inicialización del LED RGB WS2812
void init_rgb_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_RGB_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}

extern "C" void app_main(void)
{
    init_rgb_led();

    // 1. Estado Inicial: Amarillo (Inicializando)
    set_rgb_color(30, 30, 0); 
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 2. Inicializar bus UART
    pn532_bus_t *nfc_bus = pn532_uart_init(NFC_UART_PORT, PIN_NFC_TX, PIN_NFC_RX, NFC_BAUD_RATE);
    if (nfc_bus == NULL) {
        // Error en bus: Rojo parpadeante
        while(1) {
            set_rgb_color(50, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            set_rgb_color(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }

    pn532_t *nfc_device = pn532_init(nfc_bus, GPIO_NUM_NC, GPIO_NUM_NC);
    if (nfc_device == NULL) {
        set_rgb_color(50, 0, 0);
        return;
    }

    // 3. Verificar comunicación leyendo versión de firmware
    uint32_t firmware_ver = pn532_get_firmware_version(nfc_device);
    if (firmware_ver == 0) {
        // Error de ACK / Timeout: Rojo constante
        ESP_LOGE(TAG, "Fallo al comunicar con el PN532.");
        set_rgb_color(60, 0, 0); 
        return;
    }

    // 4. Inicialización Exitosa: Verde Fijo
    ESP_LOGI(TAG, "PN532 Detectado! Firmware: 0x%08lx", firmware_ver);
    set_rgb_color(0, 60, 0);

    // 5. Bucle de lectura de tarjetas
    while (1) {
        pn532_uids_array_t *detected_cards = pn532_14443_get_all_uids(nfc_device);

        if (detected_cards != NULL && detected_cards->uids_count > 0) {
            // Destello Azul al detectar tarjeta
            set_rgb_color(0, 0, 80);
            vTaskDelay(pdMS_TO_TICKS(300));

            // Volver a Verde
            set_rgb_color(0, 60, 0);
            free(detected_cards);
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
