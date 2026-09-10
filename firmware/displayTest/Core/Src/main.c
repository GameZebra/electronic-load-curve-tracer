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
#include "gui.h" // Включваме новия UI модул
#include "simulator.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define SWEEP_STEP_V   0.8


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
extern SPI_HandleTypeDef hspi1;

float simulated_voltage = 0.0;
float simulated_current = 0.0;
int selected_menu = 0; // 0=LOAD OFF, 1=SWEEP, 2=SET I, 3=SETTINGS
int selected_menu_old = 0;

typedef enum {
    STATE_INIT,
    STATE_MEASURE_DATA,
    STATE_UPDATE_GUI,
    STATE_PROCESS_MENU,
    STATE_DELAY
} SystemState_t;


#define MAX_TEST_POINTS 350
static uint32_t test_v_arr[MAX_TEST_POINTS];
static uint32_t test_i_arr[MAX_TEST_POINTS];
static uint16_t test_points_count = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
    SystemState_t current_state = STATE_INIT;

    GraphMode_t current_graph_mode = GRAPH_MODE_LINES; // Или GRAPH_MODE_LINES
    static uint16_t encoder_prev_count = 0;
    static uint8_t btn_prev_state = GPIO_PIN_SET; // Започва High заради Pull-up
    static int8_t encoder_substeps = 0; // Акумулатор за остатъчните импулси

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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  // Стартираме хардуерния енкодер на двата канала
      HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
      // Вземаме първоначалната стойност
      encoder_prev_count = __HAL_TIM_GET_COUNTER(&htim1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
        {
            switch (current_state)
            {
                case STATE_INIT:
                	GUI_InitSystem();
					Simulator_Init();

					// ТЕСТ: Смяна на симулирания панел на 10W (Voc=15V, Vmp=12V, Isc=0.8A)
					SolarPanelConfig_t test_panel = {
						.voc_mV = 22000,
						.vmp_mV = 19220,
						.isc_uA = 1500000
					};
					Simulator_SetConfig(test_panel);

                    // 1. Генерираме масива еднократно при стартиране
                    bool cycle_reset = false;
                    test_points_count = 0;

                    while (!cycle_reset && test_points_count < MAX_TEST_POINTS) {
                        Simulator_GetNextMeasurement(&test_v_arr[test_points_count],
                                                     &test_i_arr[test_points_count],
                                                     &cycle_reset);
                        if (!cycle_reset) {
                            test_points_count++;
                        }
                    }

                    current_state = STATE_UPDATE_GUI; // Директно отиваме да чертаем
                    break;

                case STATE_MEASURE_DATA:
                    // Тъй като вече имаме готовия масив, за теста тук не правим нищо.
                    // В реалния код тук ще се чете ADC-то.
                    current_state = STATE_UPDATE_GUI;
                    break;

                case STATE_UPDATE_GUI:
                    // 2. Показваме стойностите на точката с максимална мощност (Vmp = 17.5V)
                    // За теста, това е някъде в последната третина на масива
					if (test_points_count > 0) {
						uint16_t vmp_index = (test_points_count * 8) / 10;
						GUI_UpdateDashboard(test_v_arr[vmp_index], test_i_arr[vmp_index]);
					}

					// ПОДАВАМЕ РЕЖИМА current_graph_mode КАТО ПОСЛЕДЕН АРГУМЕНТ
					GUI_PlotCurveArray(test_v_arr, test_i_arr, test_points_count, current_graph_mode);

					current_state = STATE_PROCESS_MENU;
					break;
                case STATE_PROCESS_MENU:
				{
					// ==========================================
					// 1. ОТЧИТАНЕ НА ВЪРТЕНЕТО (С АКУМУЛАТОР)
					// ==========================================
					uint16_t current_count = __HAL_TIM_GET_COUNTER(&htim1);

					// Вземаме мигновената разлика от предното извикване
					int16_t step_delta = (int16_t)current_count - (int16_t)encoder_prev_count;
					encoder_prev_count = current_count; // Веднага синхронизираме брояча!

					// Добавяме новите стъпки към акумулатора
					encoder_substeps += step_delta;

					// Проверка за завъртане НАДЯСНО (напред)
					while (encoder_substeps >= 4) {
						selected_menu++;
						if (selected_menu >= MENU_COUNT) {
							selected_menu = 0;
						}
						encoder_substeps -= 4;
					}

					// Проверка за завъртане НАЛЯВО (назад)
					while (encoder_substeps <= -4) {
						selected_menu--;
						if (selected_menu < 0) {
							selected_menu = MENU_COUNT - 1;
						}
						encoder_substeps += 4;
					}

					// ==========================================
					// 2. ОТЧИТАНЕ НА БУТОНА (КЛИКВАНЕ)
					// ==========================================
					uint8_t current_btn_state = HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin);
					if (current_btn_state == GPIO_PIN_RESET && btn_prev_state == GPIO_PIN_SET) {
						// Обработка на натискането
					}
					btn_prev_state = current_btn_state;

					// Преначертаване на менюто
					GUI_ProcessMenu();
					current_state = STATE_DELAY;
					break;
				}

                case STATE_DELAY:
                    HAL_Delay(100); // Слагаме 100ms пауза (10 кадъра в секунда)
                    current_state = STATE_MEASURE_DATA;
                    break;

                default:
                    current_state = STATE_INIT;
                    break;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();

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

  /*Configure GPIO pin : ENC_BTN_Pin */
  GPIO_InitStruct.Pin = ENC_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ENC_BTN_GPIO_Port, &GPIO_InitStruct);

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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
