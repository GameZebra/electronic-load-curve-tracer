/*
 * simulator.c
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#include "simulator.h"

// Дефинирай стъпката си тук, ако не е глобално дефинирана другаде
#define SWEEP_STEP_V 0.8f

static float simulated_voltage = 0.0f;
static float simulated_current = 0.0f;

void Simulator_Init(void) {
    simulated_voltage = 0.0f;
    simulated_current = 0.0f;
}

void Simulator_GetNextMeasurement(float *voltage, float *current, bool *cycle_reset) {
    *cycle_reset = false;
    simulated_voltage += SWEEP_STEP_V;

    if (simulated_voltage > 20.0f) {
        simulated_voltage = 0.0f;
        simulated_current = 0.0f;
        *cycle_reset = true; // Индикираме, че започваме нов цикъл
    } else {
        float isc = 1.15f;
        if (simulated_voltage <= 17.5f) {
            float factor = simulated_voltage / 17.5f;
            simulated_current = isc * (1.0f - 0.05f * (factor * factor));
        } else {
            float remaining_ratio = (20.0f - simulated_voltage) / (20.0f - 17.5f);
            simulated_current = isc * 0.95f * (remaining_ratio * remaining_ratio * remaining_ratio);
        }
    }

    *voltage = simulated_voltage;
    *current = simulated_current;
}
