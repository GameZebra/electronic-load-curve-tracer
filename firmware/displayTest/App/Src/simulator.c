/*
 * simulator.c
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#include "simulator.h"

// Константи
#define SWEEP_STEP_MV    100     // 0.1V

// Вече не са макроси (#define), а променливи, които можем да променяме
static uint32_t current_voc_mV = 20000;   // По подразбиране 20.0V
static uint32_t current_vmp_mV = 17500;   // По подразбиране 17.5V
static uint32_t current_isc_uA = 1150000; // По подразбиране 1.15A

static uint32_t simulated_voltage_mV = 0;
static uint32_t simulated_current_uA = 0;

void Simulator_Init(void) {
    simulated_voltage_mV = 0;
    simulated_current_uA = 0;
    // Възстановяване на стойностите по подразбиране (20W панел)
    current_voc_mV = 20000;
    current_vmp_mV = 17500;
    current_isc_uA = 1150000;
}

// НОВО: Функция за прилагане на нова конфигурация
void Simulator_SetConfig(SolarPanelConfig_t new_config) {
    current_voc_mV = new_config.voc_mV;
    current_vmp_mV = new_config.vmp_mV;
    current_isc_uA = new_config.isc_uA;
    // Рестартираме цикъла при смяна на панела
    simulated_voltage_mV = 0;
}

void Simulator_GetNextMeasurement(uint32_t *voltage_mV, uint32_t *current_uA, bool *cycle_reset) {
    *cycle_reset = false;
    simulated_voltage_mV += SWEEP_STEP_MV;

    if (simulated_voltage_mV > current_voc_mV) {
        simulated_voltage_mV = 0;
        simulated_current_uA = 0;
        *cycle_reset = true;
    } else {
        if (simulated_voltage_mV <= current_vmp_mV) {
            uint32_t factor = (simulated_voltage_mV * 1000) / current_vmp_mV;
            uint32_t factor_sq = (factor * factor) / 1000;

            // Формула: I = Isc - (Isc * 0.05 * factor^2)
            uint32_t temp_i = (current_isc_uA / 100) * 5; // 5% от Isc
            simulated_current_uA = current_isc_uA - (temp_i * factor_sq) / 1000;
        } else { // Стръмна част: Напрежението е между Vmp и Voc
            uint32_t diff_mV = current_voc_mV - simulated_voltage_mV;
            uint32_t diff_vmp = current_voc_mV - current_vmp_mV;
            uint32_t ratio = 0;

            if(diff_vmp > 0) {
               // Отношение на оставащото напрежение към общата разлика след Vmp
               // Скалирано по 1000 (1.0 = 1000)
               ratio = (diff_mV * 1000) / diff_vmp;
            }

            // Използваме квадратно (ratio^2), а не кубично (ratio^3) за по-плавен спад
            uint32_t ratio_sq = (ratio * ratio) / 1000;

            // Токът в точката Vmp е около 95% от Isc.
            // Оттам спада параболично към 0.
            uint32_t current_at_vmp = (current_isc_uA / 100) * 95;

            simulated_current_uA = (current_at_vmp * ratio_sq) / 1000;
        }
    }

    *voltage_mV = simulated_voltage_mV;
    *current_uA = simulated_current_uA;
}
