/*
 * gui.h
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#ifndef GUI_H
#define GUI_H

#include "ili9341.h"
#include "fonts.h"
#include <stdint.h>
#include <stdbool.h>

// ==============================================================================
// ГЕОМЕТРИЯ НА РАБОТНОТО ПОЛЕ (Вътрешна видима част за графиката)
// ==============================================================================
#define GRAPH_PLOT_X        31
#define GRAPH_PLOT_Y        16
#define GRAPH_PLOT_WIDTH   179
#define GRAPH_PLOT_HEIGHT  164

#define GRAPH_AXIS_X_MIN   (GRAPH_PLOT_X - 1)
#define GRAPH_AXIS_X_MAX   (GRAPH_PLOT_X + GRAPH_PLOT_WIDTH)
#define GRAPH_AXIS_Y_MIN   (GRAPH_PLOT_Y - 1)
#define GRAPH_AXIS_Y_MAX   (GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT)

#define GRAPH_FRAME_W      (GRAPH_AXIS_X_MAX - GRAPH_AXIS_X_MIN + 1)
#define GRAPH_FRAME_H      (GRAPH_AXIS_Y_MAX - GRAPH_AXIS_Y_MIN + 1)

#define POINT_RADIUS  1

typedef enum {
    MENU_LOAD = 0,
    MENU_SWEEP,
    MENU_SET_I,
    MENU_SETTINGS,
    MENU_COUNT
} MenuIndex_t;

typedef enum {
    GRAPH_MODE_POINTS = 0,
    GRAPH_MODE_LINES  = 1
} GraphMode_t;

void Draw_Menu_Button(uint8_t index, uint8_t is_selected);
void Update_Menu_Selection(uint8_t selected_index);
void Clear_Graph_Area(void);
void Draw_Dashboard_Static(uint8_t selected_menu);
void Update_Values(float v, float i);

void GUI_InitSystem(void);
void GUI_UpdateDashboard(float voltage, float current);
void GUI_PlotPoint(float voltage, float current, GraphMode_t mode, bool reset_graph);
void GUI_ProcessMenu(void);

#endif // GUI_H
