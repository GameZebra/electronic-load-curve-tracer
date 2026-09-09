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

// Структура за параметрите на виртуалния соларен панел
typedef struct {
    uint32_t voc_mV;  // Напрежение на празен ход (Open Circuit Voltage)
    uint32_t vmp_mV;  // Напрежение при максимална мощност (Max Power Voltage)
    uint32_t isc_uA;  // Ток на късо съединение (Short Circuit Current)
} SolarPanelConfig_t;

// Инициализира симулатора с параметрите по подразбиране
void Simulator_Init(void);

// НОВО: Функция за промяна на параметрите на панела в реално време
void Simulator_SetConfig(SolarPanelConfig_t new_config);

// Взема следващата точка от симулацията
void Simulator_GetNextMeasurement(uint32_t *voltage_mV, uint32_t *current_uA, bool *cycle_reset);

#endif // SIMULATOR_H
