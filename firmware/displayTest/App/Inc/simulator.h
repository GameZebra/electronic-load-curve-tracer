/*
 * simulator.h
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>

void Simulator_Init(void);
// voltage_mV е в миливолти (напр. 17500 за 17.5V)
// current_uA е в микроампери (напр. 1150000 за 1.15A)
void Simulator_GetNextMeasurement(uint32_t *voltage_mV, uint32_t *current_uA, bool *cycle_reset);

#endif // SIMULATOR_H
