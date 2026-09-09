/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */


#include "ili9341.h"
#include "fonts.h"
#include <stdio.h> // За sprintf
#include <stdlib.h> // За функцията abs()

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// ==============================================================================
// ГЕОМЕТРИЯ НА РАБОТНОТО ПОЛЕ (Вътрешна видима част за графиката)
// ==============================================================================
#define GRAPH_PLOT_X        31    // Начало на чертането по X (вътрешна част)
#define GRAPH_PLOT_Y        16    // Начало на чертането по Y (вътрешна част)
#define GRAPH_PLOT_WIDTH   179    // Полезна ширина
#define GRAPH_PLOT_HEIGHT  164    // Полезна височина

// Изчислени координати на външните оси/рамка:
#define GRAPH_AXIS_X_MIN   (GRAPH_PLOT_X - 1)                      // 30  - Лява вертикална ос (I)
#define GRAPH_AXIS_X_MAX   (GRAPH_PLOT_X + GRAPH_PLOT_WIDTH)      // 210 - Дясна вертикална рамка
#define GRAPH_AXIS_Y_MIN   (GRAPH_PLOT_Y - 1)                      // 15  - Горна хоризонтална рамка
#define GRAPH_AXIS_Y_MAX   (GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT)     // 180 - Долна хоризонтална ос (U)

#define GRAPH_FRAME_W      (GRAPH_AXIS_X_MAX - GRAPH_AXIS_X_MIN + 1) // 181 px пълна ширина с рамката
#define GRAPH_FRAME_H      (GRAPH_AXIS_Y_MAX - GRAPH_AXIS_Y_MIN + 1) // 166 px пълна височина с рамката

#define POINT_RADIUS  1  // 0 = 1x1 пиксел; 1 = 3x3 пиксела; 2 = 5x5 пиксела


#define SWEEP_STEP_V   0.8

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
extern SPI_HandleTypeDef hspi1; // Указваме на библиотеката кой SPI ползваме

// Променливи за симулация на електронния товар
float simulated_voltage = 0.0;
float simulated_current = 0.0;
int selected_menu = 0; // 0=LOAD OFF, 1=SWEEP, 2=SET I, 3=SETTINGS
int selected_menu_old = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include <stdint.h>

// Дефиниции за менюто
typedef enum {
    MENU_LOAD = 0,
    MENU_SWEEP,
    MENU_SET_I,
    MENU_SETTINGS,
    MENU_COUNT
} MenuIndex_t;

// Структура за бутоните от менюто
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    const char *label;
} MenuItem_t;

// Тип за избор на визуализация
typedef enum {
    GRAPH_MODE_POINTS = 0,
    GRAPH_MODE_LINES  = 1
} GraphMode_t;


static const MenuItem_t menu_items[MENU_COUNT] = {
    {  5, 215, 75, 18, " LOAD OFF " },
    { 85, 215, 60, 18, " SWEEP  " },
    {150, 215, 60, 18, " SET I  " },
    {215, 215, 95, 18, "  SETTINGS " } // да добавя скорост на измерване бързо средно финно, (евентуално размер на точката)
};

// Функция за изчертаване на единичен бутон
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

// Изчертаване на цялата навигационна лента
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

    // Рамка около графиката
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, GRAPH_FRAME_W, 1, ILI9341_WHITE); // Горе
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX, GRAPH_FRAME_W, 1, ILI9341_WHITE); // Долу (Ос U)
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE); // Ляво (Ос I)
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MIN, 1, GRAPH_FRAME_H, ILI9341_WHITE); // Дясно

    // --------------------------------------------------------------------------
    // Деления и етикети по оста Y (Ток - I)
    // --------------------------------------------------------------------------
    uint16_t y_mid = (GRAPH_AXIS_Y_MIN + GRAPH_AXIS_Y_MAX) / 2;

    // Деления (Ticks) с дължина 3px наляво от оста
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MIN, 3, 1, ILI9341_LIGHTGREY); // Максимум
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, y_mid,            3, 1, ILI9341_LIGHTGREY); // Среда
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN - 3, GRAPH_AXIS_Y_MAX, 3, 1, ILI9341_LIGHTGREY); // Нула (0.0A)

    // Стойности и мерни единици
    ILI9341_WriteString(2, 2, "I[A]", Font_7x10, ILI9341_CYAN, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MIN - 3, "3.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, y_mid - 4,            "1.5", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(4, GRAPH_AXIS_Y_MAX - 5, "0.0", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // --------------------------------------------------------------------------
    // Деления и етикети по оста X (Напрежение - U)
    // --------------------------------------------------------------------------
    uint16_t x_mid = (GRAPH_AXIS_X_MIN + GRAPH_AXIS_X_MAX) / 2;

    // Деления (Ticks) с височина 3px надолу от оста
    ILI9341_FillRectangle(GRAPH_AXIS_X_MIN, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Нула (0V)
    ILI9341_FillRectangle(x_mid,            GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Среда
    ILI9341_FillRectangle(GRAPH_AXIS_X_MAX, GRAPH_AXIS_Y_MAX + 1, 1, 3, ILI9341_LIGHTGREY); // Максимум

    // Стойности и мерни единици
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
// Функция за опресняване на стойностите
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


void ILI9341_DrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy; // Грешка на отклонение

    while (1) {
        // Чертаем текущия пиксел само ако попада вътре в работното поле на графиката
        if (x0 >= GRAPH_PLOT_X && x0 <= (GRAPH_PLOT_X + GRAPH_PLOT_WIDTH - 1) &&
            y0 >= GRAPH_PLOT_Y && y0 <= (GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT - 1)) {
            ILI9341_DrawPixel(x0, y0, color);
        }

        // Достигната е крайната точка
        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  //BEGIN
  int x_graph = GRAPH_PLOT_X;
  ILI9341_Init();
  HAL_Delay(100);
  ILI9341_FillScreen(ILI9341_BLACK); // Трябва да изчисти шума и да направи екрана черен
  HAL_Delay(100);

  GraphMode_t current_graph_mode = GRAPH_MODE_POINTS;
  Draw_Dashboard_Static(0);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    	// Статични променливи за следене на предходната точка между итерациите
    	        static int prev_x = -1;
    	        static int prev_y = -1;

    	        // 1. Симулираме стъпка на измерване (Соларен панел 20W: Voc = 20V, Vmp = 17.5V)
    	                simulated_voltage += SWEEP_STEP_V;

    	                if (simulated_voltage > 20.0f) {
    	                    simulated_voltage = 0.0f;
    	                    simulated_current = 0.0f;
    	                    prev_x = -1;
    	                    prev_y = -1;
    	                    Clear_Graph_Area();
    	                } else {
    	                    // Ток на късо съединение (Isc) при 0V около 1.15A
    	                    float isc = 1.15f;

    	                    if (simulated_voltage <= 17.5f) {
    	                        // В първата част токът е почти постоянен (поведение на идеален токов източник)
    	                        // С лек спад, за да изглежда реалистично до MPP точката
    	                        float factor = simulated_voltage / 17.5f;
    	                        simulated_current = isc * (1.0f - 0.05f * (factor * factor));
    	                    } else {
    	                        // След MPP (17.5V до 20V) токът лавинообразно спада до 0 при отворена верига (Voc = 20V)
    	                        float remaining_ratio = (20.0f - simulated_voltage) / (20.0f - 17.5f);
    	                        // Използваме степен за остра крива към 0
    	                        simulated_current = isc * 0.95f * (remaining_ratio * remaining_ratio * remaining_ratio);
    	                    }
    	                }
    	        // 2. Опресняваме текста вдясно (Voltage, Current, Power)
    	        Update_Values(simulated_voltage, simulated_current);

    	        // 3. Изчисляваме координатите (чиста целочислена аритметика)
    	        // Мащабираме: 30.0V * 10 = 300 стъпки; 3.00A * 100 = 300 стъпки
    	        int v_scaled = (int)(simulated_voltage * 10);
    	        int i_scaled = (int)(simulated_current * 100);

    	        // X расте надясно спрямо напрежението
    	        int x_graph = GRAPH_PLOT_X + (v_scaled * (GRAPH_PLOT_WIDTH - 1)) / 300;

    	        // Y расте нагоре спрямо тока (обърната координата спрямо горния ляв ъгъл)
    	        int y_zero  = GRAPH_PLOT_Y + GRAPH_PLOT_HEIGHT - 1;
    	        int y_graph = y_zero - (i_scaled * (GRAPH_PLOT_HEIGHT - 1)) / 300;

    	        // Ограничаване строго вътре в работното поле
    	        if (x_graph < GRAPH_PLOT_X) x_graph = GRAPH_PLOT_X;
    	        if (x_graph > (GRAPH_AXIS_X_MAX - 1)) x_graph = GRAPH_AXIS_X_MAX - 1;
    	        if (y_graph < GRAPH_PLOT_Y) y_graph = GRAPH_PLOT_Y;
    	        if (y_graph > y_zero) y_graph = y_zero;

    	        // 4. Чертане според избрания режим
    	                if (current_graph_mode == GRAPH_MODE_LINES) {
    	                    // Режим ЛИНИИ (линейна интерполация между съседни точки)
    	                    if (prev_x != -1 && prev_y != -1) {
    	                        ILI9341_DrawLine(prev_x, prev_y, x_graph, y_graph, ILI9341_GREEN);
    	                    } else {
    	                        ILI9341_DrawPixel(x_graph, y_graph, ILI9341_GREEN);
    	                    }
    	                } else {
    	                    // Режим ТОЧКИ (правоъгълен маркер с отрязване при осите)
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

    	                // Запомняме текущата точка за линията в следващия такт
    	                prev_x = x_graph;
    	                prev_y = y_graph;

    	        // 5. Пауза и обработка на менюто
    	        HAL_Delay(10);

    	        if (selected_menu != selected_menu_old) {
    	            Update_Menu_Selection(selected_menu);
    	            selected_menu_old = selected_menu;
    	        }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ILI9341_RES_Pin|ILI9341_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ILI9341_RES_Pin ILI9341_DC_Pin ILI9341_CS_Pin */
  GPIO_InitStruct.Pin = ILI9341_RES_Pin|ILI9341_DC_Pin|ILI9341_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
