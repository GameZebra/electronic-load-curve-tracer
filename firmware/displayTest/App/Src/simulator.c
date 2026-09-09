/*
 * simulator.c
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#include "simulator.h"

// Константи в цели числа (mV и µA) спрямо оригиналните ти
#define SWEEP_STEP_MV    800     // 0.8V
#define SOLAR_VOC_MV     20000   // 20.0V
#define SOLAR_VMP_MV     17500   // 17.5V
#define SOLAR_ISC_UA     1150000 // 1.15A

static uint32_t simulated_voltage_mV = 0;
static uint32_t simulated_current_uA = 0;

void Simulator_Init(void) {
    simulated_voltage_mV = 0;
    simulated_current_uA = 0;
}

void Simulator_GetNextMeasurement(uint32_t *voltage_mV, uint32_t *current_uA, bool *cycle_reset) {
    *cycle_reset = false;
    simulated_voltage_mV += SWEEP_STEP_MV;

    if (simulated_voltage_mV > SOLAR_VOC_MV) {
        simulated_voltage_mV = 0;
        simulated_current_uA = 0;
        *cycle_reset = true;
    } else {
        if (simulated_voltage_mV <= SOLAR_VMP_MV) {
            // factor е скалиран по 1000
            uint32_t factor = (simulated_voltage_mV * 1000) / SOLAR_VMP_MV;
            uint32_t factor_sq = (factor * factor) / 1000;

            // 0.05 * 1150000 = 57500
            simulated_current_uA = SOLAR_ISC_UA - (57500 * factor_sq) / 1000;
        } else {
            uint32_t diff_mV = SOLAR_VOC_MV - simulated_voltage_mV;
            uint32_t ratio = (diff_mV * 1000) / (SOLAR_VOC_MV - SOLAR_VMP_MV);

            uint32_t ratio_sq = (ratio * ratio) / 1000;
            uint32_t ratio_cu = (ratio_sq * ratio) / 1000;

            // 0.95 * 1150000 = 1092500
            simulated_current_uA = (1092500 * ratio_cu) / 1000;
        }
    }

    *voltage_mV = simulated_voltage_mV;
    *current_uA = simulated_current_uA;
}
