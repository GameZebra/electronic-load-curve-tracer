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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Draw_Dashboard_Static(void) {
    ILI9341_FillScreen(ILI9341_BLACK);

    // Чертане на рамка около графиката чрез тънки правоъгълници (дебелина 1 пиксел)
    ILI9341_FillRectangle(5, 5, 205, 1, ILI9341_WHITE);   // Горна хоризонтална
    ILI9341_FillRectangle(5, 195, 205, 1, ILI9341_WHITE); // Долна хоризонтална
    ILI9341_FillRectangle(5, 5, 1, 190, ILI9341_WHITE);   // Лява вертикална
    ILI9341_FillRectangle(210, 5, 1, 191, ILI9341_WHITE); // Дясна вертикална

    // Текст вдясно (Статични етикети) - използваме ILI9341_WriteString
    ILI9341_WriteString(220, 10, "Voltage:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(220, 70, "Current:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);
    ILI9341_WriteString(220, 130, "Power:", Font_7x10, ILI9341_LIGHTGREY, ILI9341_BLACK);

    // Долно меню - статичен текст
    ILI9341_WriteString(10, 210, "[LOAD OFF]", Font_7x10, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(90, 210, "[SWEEP]", Font_7x10, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(150, 210, "[SET I]", Font_7x10, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(210, 210, "[SETTINGS]", Font_7x10, ILI9341_WHITE, ILI9341_BLACK);
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
  int x_graph = 10;
  ILI9341_Init();
  HAL_Delay(100);
  ILI9341_FillScreen(ILI9341_BLACK); // Трябва да изчисти шума и да направи екрана черен
  HAL_Delay(100);
  ILI9341_FillRectangle(50, 50, 100, 100, ILI9341_GREEN); // Червен квадрат в центъра
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {

//		// Превключваме пиновете на всеки 500 ms
//		HAL_GPIO_TogglePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin);
//		HAL_GPIO_TogglePin(ILI9341_CS_GPIO_Port, ILI9341_CS_Pin);
//		HAL_GPIO_TogglePin(ILI9341_DC_GPIO_Port, ILI9341_DC_Pin);
//		HAL_Delay(500);



        // 1. Симулираме промяна на данните от ADC-то
        simulated_voltage += 0.2;
        simulated_current = simulated_voltage * 0.5; // Примерна зависимост
        if(simulated_voltage > 20.0) {
            simulated_voltage = 0.0;
            x_graph = 10; // Рестартираме графиката
            ILI9341_FillRectangle(6, 6, 203, 188, ILI9341_BLACK); // Изчистваме само полето на графиката!
        }

        // 2. Опресняваме текста (забележи - екранът не мига, защото сме задали черен фон на текста)
        Update_Values(simulated_voltage, simulated_current);

        // 3. Чертаем точка от "графиката" (симулация)
        // Мащабираме V към Y координата (обърната, защото Y=0 е горе)
        int y_graph = 190 - (int)(simulated_voltage * 8);
        if(y_graph < 10) y_graph = 10;

        ILI9341_DrawPixel(x_graph, y_graph, ILI9341_RED);
        // Правим точката по-дебела за да се вижда (3x3 пиксела)
        ILI9341_FillRectangle(x_graph-1, y_graph-1, 3, 3, ILI9341_RED);

        x_graph += 2; // Местим се надясно по оста X

        // 4. Пауза за симулацията (в реалния проект тук ще четеш ADC-то)
        HAL_Delay(10);

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
