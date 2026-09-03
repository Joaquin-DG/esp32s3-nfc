#include <stdio.h>
#include "freertos/FreeRTOS.h"             // Núcleo de FreeRTOS para tipos básicos
#include "freertos/task.h"                 // Gestión de tareas y retrasos bloqueantes
#include "esp_log.h"                       // Biblioteca nativa de logs de ESP-IDF

void app_main(void)
{
    // ESP_LOGI("Inicio programa");

    while (1) {

        printf("Buscando a nemo\n");

        // Retardo pasivo de 2 segundos para no saturar el procesador
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}