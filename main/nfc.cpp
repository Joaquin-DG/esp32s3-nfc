#include <iostream>
#include "freertos/FreeRTOS.h"             // Núcleo de FreeRTOS para tipos básicos
#include "freertos/task.h"                 // Gestión de tareas y retrasos bloqueantes
#include "pn532.h"
#include "esp_log.h"                       // Biblioteca nativa de logs de ESP-IDF

extern "C" void app_main(void)
{
    // ESP_LOGI("Inicio programa");

    

    while (1) {

        std::cout << "Diselo mike" << std::endl;

        // Retardo pasivo de 2 segundos para no saturar el procesador
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}