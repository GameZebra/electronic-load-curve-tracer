/*
 * gui.c
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */


#include "gui.h"
#include <stdio.h> // За sprintf

// Структура за бутоните от менюто
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    const char *label;
} MenuItem_t;

static const MenuItem_t menu_items[MENU_COUNT] = {
    {  5, 215, 75, 18, " LOAD OFF " },
    { 85, 215, 60, 18, " SWEEP  " },
    {150, 215, 60, 18, " SET I  " },
    {215, 215, 95, 18, "  SETTINGS " }
};


// Статични променливи, които пазят състоянието само в рамките на GUI модула
static int prev_x = -1;
static int prev_y = -1;
static int selected_menu_old = -1;

// Ако selected_menu се променя от прекъсване на енкодер/бутон, може да е extern
extern int selected_menu;

void Draw_Menu_Button(uint8_t index, uint8_t is_selected) {
    const MenuItem_t *btn = &menu_items[index];

    // Инвертиране на цветовете при селекция
    uint16_t bg_color   = is_selected ? ILI9341_WHITE : ILI9341_BLACK;
    uint16_t text_color = is_selected ? ILI9341_BLACK : ILI9341_LIGHTGREY;
    uint16_t border_col = is_selected ? ILI9341_YELLOW : ILI9341_DARKGREY;

    // Фон на бутона
    ILI9341_FillRectangle(btn->x, btn->y, btn->w, btn->h, bg_color);

    // Текст (центриран по височина с офсет 4px)
    ILI9341_WriteString(btn->x + 4, btn->y + 4, btn->label, Font_7x10, text_color, bg_color);

    // Рамка около бутона
    ILI9341_FillRectangle(btn->x, btn->y, btn->w, 1, border_col);                     // горе
    ILI9341_FillRectangle(btn->x, btn->y + btn->h - 1, btn->w, 1, border_col);         // долу
    ILI9341_FillRectangle(btn->x, btn->y, 1, btn->h, border_col);                     // ляво
    ILI9341_FillRectangle(btn->x + btn->w - 1, btn->y, 1, btn->h, border_col);         // дясно
}

void Update_Menu_Selection(uint8_t selected_index) {
    for (uint8_t i = 0; i < MENU_COUNT; i++) {
        Draw_Menu_Button(i, (i == selected_index));
    }
}

void Clear_Graph_Area(void) {
    // Запълва с черно САМО вътрешността на координатното поле
    ILI9341_FillRectangle(GRAPH_PLOT_X, GRAPH_PLOT_Y,
                          GRAPH_PLOT_WIDTH, GRAPH_PLOT_HEIGHT,
                          ILI9341_BLACK);
}

void Draw_Dashboard_Static(uint8_t selected_menu) {
    ILI9341_FillScreen(ILI9341_BLACK);

    // ==========================================================================
    // 1. КООРДИНАТНА СИСТЕМА И РАМКА
    // ==========================================================================
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, GRAPH_FRAME_W, 1, ILI9341_WHITE); // Горе
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX, GRAPH_FRAME_W, 1, ILI9341_WHITE); // Долу (Ос U)
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE); // Ляво (Ос I)
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE); // Дясно

    // --------------------------------------------------------------------------
    // Деления и етикети по оста Y (Ток - I)
    // --------------------------------------------------------------------------
    uint16_t y_mid = (GRAPH_AXIS_Y_MIN + GRAPH_AXIS_Y_MAX) / 2;
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MIN, 3, 1, ILI9341_LIGHTGREY); // Максимум
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, y_mid,            3, 1, ILI9341_LIGHTGREY); // Среда
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MAX, 3, 1, ILI9341_LIGHTGREY); // Нула (0.0A)

    ILI9341_WriteString(2, 2, "I[A]", Font_7x10, ILI9341_CYAN, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MIN - 3, "3.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, y_mid - 4,            "1.5", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MAX - 5, "0.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // --------------------------------------------------------------------------
    // Деления и етикети по оста X (Напрежение - U)
    // --------------------------------------------------------------------------
    uint16_t x_mid = (GRAPH_AXIS_X_MIN + GRAPH_AXIS_X_MAX) / 2;
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Нула (0V)
    ILI9341_FillRectangle(x_mid,            GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Среда
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Максимум

    ILI9341_WriteString(GRAPH_AXIS_X_MIN - 2, GRAPH_AXIS_Y_MAX + 6, "0V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(x_mid - 10,           GRAPH_AXIS_Y_MAX + 6, "15V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX - 18, GRAPH_AXIS_Y_MAX + 6, "30V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(x_mid - 10,           GRAPH_AXIS_Y_MAX + 19, "U [V]", Font_7x10, ILI9341_CYAN, ILI9341_BLACK);

    // ==========================================================================
    // 2. СТАТИЧНИ ЕТИКЕТИ В ДЯСНО
    // ==========================================================================
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 10,  "Voltage:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 70,  "Current:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 130, "Power:",   Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // ==========================================================================
    // 3. ДОЛНО МЕНЮ
    // ==========================================================================
    Update_Menu_Selection(selected_menu);
}

void Update_Values(float v, float i) {
    char buffer[16];

    // Напрежение (жълто)
    int v_whole = (int)v;
    int v_decimal = (int)(v * 10) % 10;
    sprintf(buffer, "%2d.%1d V ", v_whole, v_decimal);
    ILI9341_WriteString(220, 25, buffer, Font_11x18, ILI9341_YELLOW, ILI9341_BLACK);

    // Ток (светло синьо)
    int i_whole = (int)i;
    int i_decimal = (int)(i * 10) % 10;
    sprintf(buffer, "%2d.%1d A ", i_whole, i_decimal);
    ILI9341_WriteString(220, 85, buffer, Font_11x18, ILI9341_CYAN, ILI9341_BLACK);

    // Мощност (зелено)
    float p = v * i;
    int p_whole = (int)p;
    int p_decimal = (int)(p * 10) % 10;
    sprintf(buffer, "%3d.%1d W ", p_whole, p_decimal);
    ILI9341_WriteString(220, 145, buffer, Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
}


void GUI_InitSystem(void) {
    ILI9341_Init();
    HAL_Delay(100);
    ILI9341_FillScreen(ILI9341_BLACK);
    HAL_Delay(100);
    Draw_Dashboard_Static(0); // Предполагам тази функция вече я имаш в gui.c
}

void GUI_UpdateDashboard(float voltage, float current) {
    Update_Values(voltage, current);
}

void GUI_PlotPoint(float voltage, float current, GraphMode_t mode, bool reset_graph) {
    if (reset_graph) {
        prev_x = -1;
        prev_y = -1;
        Clear_Graph_Area();
        return; // Излизаме, няма какво да чертаем при ресет
    }

    // 3. Изчисляваме координатите
    int v_scaled = (int)(voltage * 10);
    int i_scaled = (int)(current * 100);

    int x_graph = GRAPH_PLOT_X + (v_scaled * (GRAPH_PLOT_WIDTH - 1)) / 300;
    int y_zero  = GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT - 1;
    int y_graph = y_zero - (i_scaled * (GRAPH_PLOT_HEIGHT - 1)) / 300;

    if (x_graph < GRAPH_PLOT_X) x_graph = GRAPH_PLOT_X;
    if (x_graph > (GRAPH_AXIS_X_MAX - 1)) x_graph = GRAPH_AXIS_X_MAX - 1;
    if (y_graph < GRAPH_PLOT_Y) y_graph = GRAPH_PLOT_Y;
    if (y_graph > y_zero) y_graph = y_zero;

    // 4. Чертане според избрания режим
    if (mode == GRAPH_MODE_LINES) {
        if (prev_x != -1 && prev_y != -1) {
            ILI9341_DrawLine(prev_x, prev_y, x_graph, y_graph, ILI9341_GREEN);
        } else {
            ILI9341_DrawPixel(x_graph, y_graph, ILI9341_GREEN);
        }
    } else {
        int x_start = (x_graph - POINT_RADIUS < GRAPH_PLOT_X) ? GRAPH_PLOT_X : (x_graph - POINT_RADIUS);
        int y_start = (y_graph - POINT_RADIUS < GRAPH_PLOT_Y) ? GRAPH_PLOT_Y : (y_graph - POINT_RADIUS);
        int x_end   = (x_graph + POINT_RADIUS > GRAPH_AXIS_X_MAX - 1) ? (GRAPH_AXIS_X_MAX - 1) : (x_graph + POINT_RADIUS);
        int y_end   = (y_graph + POINT_RADIUS > y_zero) ? y_zero : (y_graph + POINT_RADIUS);

        int w = x_end - x_start + 1;
        int h = y_end - y_start + 1;

        if (w > 0 && h > 0) {
            ILI9341_FillRectangle(x_start, y_start, w, h, ILI9341_GREEN);
        }
    }

    prev_x = x_graph;
    prev_y = y_graph;
}

void GUI_ProcessMenu(void) {
    if (selected_menu != selected_menu_old) {
        Update_Menu_Selection(selected_menu);
        selected_menu_old = selected_menu;
    }
}





