/*
 * simulator.h
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>

// Инициализира началните стойности на симулатора
void Simulator_Init(void);

// Генерира следващата точка от V-I характеристиката
// Връща true в cycle_reset, когато кривата стигне края и трябва да започне отначало
void Simulator_GetNextMeasurement(float *voltage, float *current, bool *cycle_reset);

#endif // SIMULATOR_H
