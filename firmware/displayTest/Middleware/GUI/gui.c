/*
 * gui.c
 *
 *  Created on: Sep 9, 2026
 *      Author: gamezebra
 */

#include "gui.h"

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

// Статични променливи
static int prev_x = -1;
static int prev_y = -1;
static int selected_menu_old = -1;


// Максимален брой точки за графиката (спрямо ширината на полето)
#define MAX_GRAPH_POINTS GRAPH_PLOT_WIDTH

static int16_t prev_curve_x[MAX_GRAPH_POINTS] = {0};
static int16_t prev_curve_y[MAX_GRAPH_POINTS] = {0};
static uint16_t prev_points_count = 0;


extern int selected_menu;

// ==============================================================================
// ПОМОЩНИ ВЪТРЕШНИ ФУНКЦИИ (трябва да са преди функциите, които ги извикват)
// ==============================================================================

static void Format_Value_String(uint32_t whole, uint32_t frac_2digits, const char* suffix, char* buffer) {
    int idx = 0;

    // 1. Конвертиране на цялата част
    if (whole == 0) {
        buffer[idx++] = '0';
    } else {
        char temp[10];
        int t_idx = 0;
        while (whole > 0) {
            temp[t_idx++] = '0' + (whole % 10);
            whole /= 10;
        }
        while (t_idx > 0) {
            buffer[idx++] = temp[--t_idx];
        }
    }

    // 2. Добавяме десетичната точка
    buffer[idx++] = '.';

    // 3. Добавяме дробната част (гарантирано 2 цифри)
    buffer[idx++] = '0' + (frac_2digits / 10);
    buffer[idx++] = '0' + (frac_2digits % 10);

    // 4. Добавяме интервал и суфикса (" V", " A" и т.н.)
    buffer[idx++] = ' ';
    while (*suffix != '\0') {
        buffer[idx++] = *suffix++;
    }

    // 5. НОВО: Допълваме с интервали до фиксирана дължина (напр. 8 символа),
    // за да изтрием гарантирано старите остатъци от екрана.
    while (idx < 8) {
        buffer[idx++] = ' ';
    }

    // 6. Задължително терминираме стринга
    buffer[idx] = '\0';
}

static uint32_t Calculate_Power_mW(uint32_t voltage_mV, uint32_t current_uA) {
    uint32_t current_mA = current_uA / 1000;
    return (voltage_mV * current_mA) / 1000;
}

// ==============================================================================
// ПУБЛИЧНИ ФУНКЦИИ НА GUI МОДУЛА
// ==============================================================================

void Draw_Menu_Button(uint8_t index, uint8_t is_selected) {
    const MenuItem_t *btn = &menu_items[index];
    uint16_t bg_color   = is_selected ? ILI9341_WHITE : ILI9341_BLACK;
    uint16_t text_color = is_selected ? ILI9341_BLACK : ILI9341_LIGHTGREY;
    uint16_t border_col = is_selected ? ILI9341_YELLOW : ILI9341_DARKGREY;

    ILI9341_FillRectangle(btn->x, btn->y, btn->w, btn->h, bg_color);
    ILI9341_WriteString(btn->x + 4, btn->y + 4, btn->label, Font_7x10, text_color, bg_color);

    ILI9341_FillRectangle(btn->x, btn->y, btn->w, 1, border_col);
    ILI9341_FillRectangle(btn->x, btn->y + btn->h - 1, btn->w, 1, border_col);
    ILI9341_FillRectangle(btn->x, btn->y, 1, btn->h, border_col);
    ILI9341_FillRectangle(btn->x + btn->w - 1, btn->y, 1, btn->h, border_col);
}

void Update_Menu_Selection(uint8_t selected_index) {
    for (uint8_t i = 0; i < MENU_COUNT; i++) {
        Draw_Menu_Button(i, (i == selected_index));
    }
}

void Clear_Graph_Area(void) {
    ILI9341_FillRectangle(GRAPH_PLOT_X, GRAPH_PLOT_Y,
                          GRAPH_PLOT_WIDTH, GRAPH_PLOT_HEIGHT,
                          ILI9341_BLACK);
}

void Draw_Dashboard_Static(uint8_t selected_menu) {
    ILI9341_FillScreen(ILI9341_BLACK);

    // 1. КООРДИНАТНА СИСТЕМА И РАМКА
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, GRAPH_FRAME_W, 1, ILI9341_WHITE);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX, GRAPH_FRAME_W, 1, ILI9341_WHITE);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE);

    // Деления по оста Y
    uint16_t y_mid = (GRAPH_AXIS_Y_MIN + GRAPH_AXIS_Y_MAX) / 2;
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MIN, 3, 1, ILI9341_LIGHTGREY);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, y_mid,            3, 1, ILI9341_LIGHTGREY);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MAX, 3, 1, ILI9341_LIGHTGREY);

    ILI9341_WriteString(2, 2, "I[A]", Font_7x10, ILI9341_CYAN, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MIN - 3, "3.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, y_mid - 4,            "1.5", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MAX - 5, "0.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // Деления по оста X
    uint16_t x_mid = (GRAPH_AXIS_X_MIN + GRAPH_AXIS_X_MAX) / 2;
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY);
    ILI9341_FillRectangle(x_mid,            GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY);
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY);

    ILI9341_WriteString(GRAPH_AXIS_X_MIN - 2, GRAPH_AXIS_Y_MAX + 6, "0V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(x_mid - 10,           GRAPH_AXIS_Y_MAX + 6, "15V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX - 18, GRAPH_AXIS_Y_MAX + 6, "30V", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(x_mid - 10,           GRAPH_AXIS_Y_MAX + 19, "U [V]", Font_7x10, ILI9341_CYAN, ILI9341_BLACK);

    // 2. СТАТИЧНИ ЕТИКЕТИ В ДЯСНО
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 10,  "Voltage:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 70,  "Current:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(GRAPH_AXIS_X_MAX + 10, 130, "Power:",   Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // 3. ДОЛНО МЕНЮ
    Update_Menu_Selection(selected_menu);
}

void GUI_InitSystem(void) {
    ILI9341_Init();
    HAL_Delay(100);
    ILI9341_FillScreen(ILI9341_BLACK);
    HAL_Delay(100);
    Draw_Dashboard_Static(0);
}

void GUI_UpdateDashboard(uint32_t voltage_mV, uint32_t current_uA) {
    char v_str[16];
    char i_str[16];
    char p_str[16];

    uint32_t v_whole = voltage_mV / 1000;
    uint32_t v_frac  = (voltage_mV % 1000) / 10;
    Format_Value_String(v_whole, v_frac, "V", v_str);

    uint32_t i_whole = current_uA / 1000000;
    uint32_t i_frac  = (current_uA % 1000000) / 10000;
    Format_Value_String(i_whole, i_frac, "A", i_str);

    uint32_t power_mW = Calculate_Power_mW(voltage_mV, current_uA);
    uint32_t w_whole = power_mW / 1000;
    uint32_t w_frac  = (power_mW % 1000) / 10;
    Format_Value_String(w_whole, w_frac, "W", p_str);

     ILI9341_WriteString(220, 25, v_str, Font_11x18, ILI9341_YELLOW, ILI9341_BLACK);
     ILI9341_WriteString(220, 85, i_str, Font_11x18, ILI9341_CYAN, ILI9341_BLACK);
     ILI9341_WriteString(220, 145, p_str, Font_11x18, ILI9341_GREEN, ILI9341_BLACK);
}

void GUI_PlotPoint(uint32_t voltage_mV, uint32_t current_uA, GraphMode_t mode, bool reset_graph) {
    if (reset_graph) {
        prev_x = -1;
        prev_y = -1;
        Clear_Graph_Area();
        return;
    }

    int v_scaled = voltage_mV / 100;
    int i_scaled = current_uA / 10000;

    int x_graph = GRAPH_PLOT_X + (v_scaled * (GRAPH_PLOT_WIDTH - 1)) / 300;
    int y_zero  = GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT - 1;
    int y_graph = y_zero - (i_scaled * (GRAPH_PLOT_HEIGHT - 1)) / 300;

    if (x_graph < GRAPH_PLOT_X) x_graph = GRAPH_PLOT_X;
    if (x_graph > (GRAPH_AXIS_X_MAX - 1)) x_graph = GRAPH_AXIS_X_MAX - 1;
    if (y_graph < GRAPH_PLOT_Y) y_graph = GRAPH_PLOT_Y;
    if (y_graph > y_zero) y_graph = y_zero;

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


// Функция, която приема готови масиви с напрежение (mV) и ток (uA)
void GUI_PlotCurveArray(uint32_t *voltage_mV_arr, uint32_t *current_uA_arr, uint16_t points_count, GraphMode_t mode) {
    if (points_count > MAX_GRAPH_POINTS) points_count = MAX_GRAPH_POINTS;

    int16_t current_curve_x[MAX_GRAPH_POINTS];
    int16_t current_curve_y[MAX_GRAPH_POINTS];

    // ИЗНАСЯМЕ y_zero ТУК, за да е глобална за цялата функция
    int y_zero = GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT - 1;

    // ЕТАП 1: Изчисляване и Clipping (ограничаване)
    for (uint16_t i = 0; i < points_count; i++) {
        int v_scaled = voltage_mV_arr[i] / 100;
        int i_scaled = current_uA_arr[i] / 10000;

        int x = GRAPH_PLOT_X + (v_scaled * (GRAPH_PLOT_WIDTH - 1)) / 300;
        int y = y_zero - (i_scaled * (GRAPH_PLOT_HEIGHT - 1)) / 300;

        if (x < GRAPH_PLOT_X) x = GRAPH_PLOT_X;
        if (x > (GRAPH_AXIS_X_MAX - 1)) x = GRAPH_AXIS_X_MAX - 1;
        if (y < GRAPH_PLOT_Y) y = GRAPH_PLOT_Y;
        if (y > y_zero) y = y_zero;

        current_curve_x[i] = x;
        current_curve_y[i] = y;
    }

    // ЕТАП 2: Диф-изтриване на СТАРАТА крива (с цвета на фона)
    if (prev_points_count > 1) {
        for (uint16_t i = 0; i < prev_points_count; i++) {
            if (mode == GRAPH_MODE_LINES) {
                if (i < prev_points_count - 1) {
                     ILI9341_DrawLine(prev_curve_x[i], prev_curve_y[i],
                                      prev_curve_x[i+1], prev_curve_y[i+1],
                                      ILI9341_BLACK);
                }
            } else { // GRAPH_MODE_POINTS
                // Изтриваме пиксела (или квадратчето)
                int x_start = (prev_curve_x[i] - POINT_RADIUS < GRAPH_PLOT_X) ? GRAPH_PLOT_X : (prev_curve_x[i] - POINT_RADIUS);
                int y_start = (prev_curve_y[i] - POINT_RADIUS < GRAPH_PLOT_Y) ? GRAPH_PLOT_Y : (prev_curve_y[i] - POINT_RADIUS);
                int x_end   = (prev_curve_x[i] + POINT_RADIUS > GRAPH_AXIS_X_MAX - 1) ? (GRAPH_AXIS_X_MAX - 1) : (prev_curve_x[i] + POINT_RADIUS);
                int y_end   = (prev_curve_y[i] + POINT_RADIUS > y_zero) ? y_zero : (prev_curve_y[i] + POINT_RADIUS);

                int w = x_end - x_start + 1;
                int h = y_end - y_start + 1;
                if (w > 0 && h > 0) {
                     ILI9341_FillRectangle(x_start, y_start, w, h, ILI9341_BLACK);
                }
            }
        }
    }

    // ЕТАП 3: Изчертаване на НОВАТА крива
    if (points_count > 1) {
        for (uint16_t i = 0; i < points_count; i++) {
            if (mode == GRAPH_MODE_LINES) {
                if (i < points_count - 1) {
                     ILI9341_DrawLine(current_curve_x[i], current_curve_y[i],
                                      current_curve_x[i+1], current_curve_y[i+1],
                                      ILI9341_GREEN);
                }
            } else { // GRAPH_MODE_POINTS
                int x_start = (current_curve_x[i] - POINT_RADIUS < GRAPH_PLOT_X) ? GRAPH_PLOT_X : (current_curve_x[i] - POINT_RADIUS);
                int y_start = (current_curve_y[i] - POINT_RADIUS < GRAPH_PLOT_Y) ? GRAPH_PLOT_Y : (current_curve_y[i] - POINT_RADIUS);
                int x_end   = (current_curve_x[i] + POINT_RADIUS > GRAPH_AXIS_X_MAX - 1) ? (GRAPH_AXIS_X_MAX - 1) : (current_curve_x[i] + POINT_RADIUS);
                int y_end   = (current_curve_y[i] + POINT_RADIUS > y_zero) ? y_zero : (current_curve_y[i] + POINT_RADIUS);

                int w = x_end - x_start + 1;
                int h = y_end - y_start + 1;
                if (w > 0 && h > 0) {
                     ILI9341_FillRectangle(x_start, y_start, w, h, ILI9341_GREEN);
                }
            }
        }
    }

    // ЕТАП 4: Копиране в prev масива
    for (uint16_t i = 0; i < points_count; i++) {
        prev_curve_x[i] = current_curve_x[i];
        prev_curve_y[i] = current_curve_y[i];
    }
    prev_points_count = points_count;
}
