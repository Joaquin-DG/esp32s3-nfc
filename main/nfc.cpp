#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "data.hpp"

// 1. Descomenta y sustituye por ssid y password correspondiente a tu configuracion
// #define WIFI_SSID           "test_ssid"
// #define WIFI_PASS           "test_password"
// 
//  2. URL de prueba (puedes usar un canal propio en ntfy.sh para recibir la notificación en tu móvil)
// #define HTTP_POST_URL       "https://example.com"

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static const char *TAG = "HTTP_POST_TEST";
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

// Manejador de eventos de conexión Wi-Fi e IP
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Reintentando conexion al punto de acceso...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGE(TAG, "Fallo el intento de conexion Wi-Fi");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Direccion IP asignada: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Inicialización de la pila de red Wi-Fi
static void wifi_init_station(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Conectando a la red Wi-Fi...");

    // Espera hasta obtener IP o agotar los reintentos
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conexion Wi-Fi establecida correctamente.");
    } else {
        ESP_LOGE(TAG, "Error grave: No se pudo establecer la conexion Wi-Fi.");
    }
}

// Función encargada de realizar la solicitud HTTP POST
static void realizar_http_post(const char* mensaje)
{
    esp_http_client_config_t config = {};
    config.url = HTTP_POST_URL;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo asignar memoria para el cliente HTTP");
        return;
    }

    // Configuración del método y cabeceras de la petición
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_http_client_set_header(client, "Title", "Esp32-s3");

    // Asignación del cuerpo (payload)
    esp_http_client_set_post_field(client, mensaje, strlen(mensaje));

    // Ejecución sincrónica de la transferencia
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int64_t content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "Peticion POST exitosa. HTTP Status = %d, Content-Length = %lld",
                 status_code, content_length);
    } else {
        ESP_LOGE(TAG, "Fallo la ejecucion de la petición HTTP POST: %s", esp_err_to_name(err));
    }

    // Liberación de recursos
    esp_http_client_cleanup(client);
}

extern "C" void app_main(void)
{
    // Inicialización del almacenamiento de credenciales NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Paso 1: Conectar a Wi-Fi
    wifi_init_station();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // Paso 2: Enviar petición HTTP POST
    ESP_LOGI(TAG, "Enviando mensaje POST de prueba...");
    realizar_http_post("¡Hola! Esta es una prueba de peticion HTTP POST desde mi ESP32-S3.");
}