/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include <stdbool.h>
#include <stdlib.h>
// #include "stm32f10x_flash.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define GPIO_SET_PIN(port, pin) ((port)->BSRR = (pin))
#define GPIO_CLEAR_PIN(port, pin) ((port)->BSRR = (pin << 16u))

#define DUTY 195
// #define DUTY 0

// these timings shoud be randomised a bit, to avoid stroboscopic effect when
// filming
#define DELAY1 469  // smth about 500
#define DELAY2 2047 // smth about 2000

#define TEST_FUNC_DELAY 10

#define DEC_BIT0 GPIOA, GPIO_PIN_5
#define DEC_BIT1 GPIOB, GPIO_PIN_1
#define DEC_BIT2 GPIOB, GPIO_PIN_10
#define DEC_BIT3 GPIOA, GPIO_PIN_7

#define RGB_NUM 4
#define PWM_MIN 0
#define PWM_HIGH 7
#define PWM_LOW 2
#define RGB_OFFSET 100

#define NEON_ORANGE 0xFF6700
#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define ROYAL_FUCHSIA 0xCA2C92
#define POMEGRANATE 0xF34723
#define MINT 0x3EB489
#define CORNFLOWER_BLUE 0x6495ED
#define BULGARIAN_ROSE 0x480607
#define BYZANTINE 0xBD33A4
#define AMBER 0xFFBF00
#define PERSIAN_INDIGO 0x32127A
#define BLACK 0x000000

#define NEON_ORANGE_BLINK 14
#define RANDOM_BLINK 15
#define NUMBER_OF_COLORS 12 // except black which is a zero element of array
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
_Bool irq_flag = 0;

uint8_t a, b, c, d; // digits
_Bool sp; // flag that ensures that test() func will be called single time
          // during a minute if the conditions for calling test() are met
uint16_t i; // tick counter in while(1) to set button press delay
extern volatile uint8_t num; // anode state machine variable in while(1)
uint32_t get_tick_prev =
    0; // prev timestamp state variable for nonblocking wait in while(1) loop
       // for working with "+" and "-" buttons
uint32_t get_tick_prev_btn3 =
    0; // prev timestamp state variable for nonblocking wait in while(1) loop
       // for working with "RGB" button
uint32_t _test_tick_prev =
    0; // prev timestamp state variable for nonblocking wait in test() func
uint32_t tmp_minutes = 0; // the whole day's tmp minutes counter for correct
                          // work of the time subtraction code in while(1)
volatile uint32_t tick_us = 0;
_Bool show_rgb_demo = true;
uint32_t rgb_demo_color = NEON_ORANGE;
uint8_t tmp_color_counter = 1;

typedef struct {
  uint32_t color;
  uint8_t top_point_delay_step;
  uint32_t get_tick_prev_us; // prev timestamp state variable for nonblocking
                             // wait in rgb_demo(...)
  uint8_t current_led_num;
  _Bool init;
} RGBDemoStateMachine;

RGBDemoStateMachine rgb_demo_state_machine;

uint8_t rgb_mode = 0;
uint32_t rgb_mode_colors[256] = {
    BLACK,          RED,           GREEN,
    BLUE,           ROYAL_FUCHSIA, NEON_ORANGE,
    POMEGRANATE,    MINT,          CORNFLOWER_BLUE,
    BULGARIAN_ROSE, BYZANTINE,     AMBER,
    PERSIAN_INDIGO,
};

uint16_t dma_pwm_buff[RGB_NUM * 24 + RGB_OFFSET] = {
    0,
};
uint32_t colors_buf[RGB_NUM] = {
    0,
};
uint32_t grb_colors_buf[RGB_NUM] = {
    0,
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

volatile uint32_t my_random(uint32_t min, uint32_t max);
void decoder_write(uint8_t num);
__STATIC_INLINE uint32_t GetTick_us();
void all_clear();
__STATIC_INLINE void wait(__IO uint32_t us);
void anode_switch(uint8_t num);
void test();
void grb_to_pwm(uint32_t *grb_buf, uint16_t *out_buf);
__STATIC_INLINE void bitbang_rgb_write(uint16_t *in_buf, int len);
void rgb_to_grb(uint32_t *rgb_buf, uint32_t *grb_buf);
void rgb_demo(RGBDemoStateMachine *states, uint32_t in_color,
              uint32_t grayscale_num, uint32_t grayscale_shift,
              uint32_t step_delay);
void reset_rgb_demo_states(RGBDemoStateMachine *states);
void set_array_with_color(uint32_t *rgb_buf, uint32_t color);
void SetReadOutProtection();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_RTCEx_TamperIRQHandler(&hrtc);
  for (int i = 0; i < DUTY; i++) {
    TIM1->CCR2 = i;
    HAL_Delay(10);
  }
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  test();
  rgb_to_grb(colors_buf, grb_colors_buf);
  grb_to_pwm(grb_colors_buf, dma_pwm_buff);
  rgb_mode = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);

  SetReadOutProtection();

  // HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_4, (uint32_t*)dma_pwm_buff,
  // RGB_NUM*24+RGB_OFFSET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    if (rgb_mode == NEON_ORANGE_BLINK) {
      show_rgb_demo = true;
      rgb_demo_color = NEON_ORANGE;

    } else if (rgb_mode == RANDOM_BLINK) {
      show_rgb_demo = true;
    }
    if (show_rgb_demo) {
      rgb_demo(&rgb_demo_state_machine, rgb_demo_color, 100, 5, 10);
      if (!rgb_demo_state_machine.init &&
          rgb_demo_state_machine.current_led_num < 3) {
        rgb_demo_state_machine.current_led_num++;
        if (rgb_mode == RANDOM_BLINK) {
          if (tmp_color_counter < NUMBER_OF_COLORS + 1) {
            tmp_color_counter++;
          }
          if (tmp_color_counter >= NUMBER_OF_COLORS + 1) {
            tmp_color_counter = my_random(1, NUMBER_OF_COLORS);
          }
          rgb_demo_color = rgb_mode_colors[tmp_color_counter];
        }
      } else if (!rgb_demo_state_machine.init &&
                 rgb_demo_state_machine.current_led_num >= 3) {
        rgb_demo_state_machine.current_led_num = 0;
        show_rgb_demo = false;

        if (rgb_mode <= NUMBER_OF_COLORS) {
          set_array_with_color(colors_buf, rgb_mode_colors[rgb_mode]);
          rgb_to_grb(colors_buf, grb_colors_buf);
          grb_to_pwm(grb_colors_buf, dma_pwm_buff);
          bitbang_rgb_write(dma_pwm_buff, RGB_NUM * 24 + RGB_OFFSET);
        }
      }
    }

    // bitbang_rgb_write(dma_pwm_buff, RGB_NUM*24+RGB_OFFSET);
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    d = sTime.Hours / 10;
    c = sTime.Hours % 10;
    b = sTime.Minutes / 10;
    a = sTime.Minutes % 10;
    if (irq_flag == 1) {
      anode_switch(num);
      irq_flag = 0;
    }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == 0) {
      if (i < 5) {
        if ((HAL_GetTick() - get_tick_prev) > 250) {
          sTime.Minutes++;
          HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
          HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
          get_tick_prev = HAL_GetTick();
          i++;
        }
      } else {
        if ((HAL_GetTick() - get_tick_prev) > 20) {
          sTime.Minutes++;
          HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
          HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
          get_tick_prev = HAL_GetTick();
          i++;
        }
      }
    } else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == 0) {
      if (i < 5) {
        if ((HAL_GetTick() - get_tick_prev) > 250) {
          tmp_minutes--;
          if (tmp_minutes > 1439) {
            tmp_minutes = 1439;
          }
          sTime.Hours = tmp_minutes / 60;
          sTime.Minutes = tmp_minutes % 60;
          HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
          HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
          get_tick_prev = HAL_GetTick();
          i++;
        }
      } else {
        if ((HAL_GetTick() - get_tick_prev) > 20) {
          tmp_minutes--;
          if (tmp_minutes > 1439) {
            tmp_minutes = 1439;
          }
          sTime.Hours = tmp_minutes / 60;
          sTime.Minutes = tmp_minutes % 60;
          HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
          HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
          get_tick_prev = HAL_GetTick();
          i++;
        }
      }
    } else {
      i = 0;
      tmp_minutes = sTime.Minutes + sTime.Hours * 60;
    }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == 0) {
      if ((HAL_GetTick() - get_tick_prev_btn3) > 300) {
        rgb_mode++;
        if (rgb_mode > RANDOM_BLINK) {
          rgb_mode = 0;
          show_rgb_demo = false;
          reset_rgb_demo_states(&rgb_demo_state_machine);
          set_array_with_color(colors_buf, BLACK);
        }
        if (rgb_mode <= NUMBER_OF_COLORS) {
          set_array_with_color(colors_buf, rgb_mode_colors[rgb_mode]);
        } else if (rgb_mode == NUMBER_OF_COLORS + 1) { // special multicolor
                                                       // mode
          colors_buf[0] = CORNFLOWER_BLUE;
          colors_buf[1] = NEON_ORANGE;
          colors_buf[2] = MINT;
          colors_buf[3] = BYZANTINE;
        } else if (rgb_mode > NUMBER_OF_COLORS + 1 &&
                   rgb_mode <= NEON_ORANGE_BLINK) {
          reset_rgb_demo_states(&rgb_demo_state_machine);
          set_array_with_color(colors_buf, BLACK);
        }
        HAL_PWR_EnableBkUpAccess();
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, rgb_mode);
        // HAL_PWR_DisableBkUpAccess();
        rgb_to_grb(colors_buf, grb_colors_buf);
        grb_to_pwm(grb_colors_buf, dma_pwm_buff);
        bitbang_rgb_write(dma_pwm_buff, RGB_NUM * 24 + RGB_OFFSET);
        get_tick_prev_btn3 = HAL_GetTick();
      }
    }

    if ((sTime.Minutes % 10 == 5) && (sp == 0) &&
        (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)) &&
        (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))) {
      test();
      sp = 1;
    }
    if (sTime.Minutes % 10 != 5) {
      sp = 0;
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
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief RTC Initialization Function
 * @param None
 * @retval None
 */
static void MX_RTC_Init(void) {

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
   */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 8;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 255;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void) {

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 255;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(
      GPIOA, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
      GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10,
                    GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

volatile uint32_t my_random(uint32_t min, uint32_t max) {
  srand(tick_us);
  return min + (uint32_t)rand() / (RAND_MAX / (max - min + 1) + 1);
}

void decoder_write(uint8_t num) {
  switch (num) {
  case 0:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 1:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 2:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 3:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 4:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 5:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 6:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 7:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_RESET);
    break;
  case 8:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_SET);
    break;
  case 9:
    HAL_GPIO_WritePin(DEC_BIT0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DEC_BIT1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT2, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DEC_BIT3, GPIO_PIN_SET);
    break;
  }
}

__STATIC_INLINE uint32_t GetTick_us() { return tick_us; }

void all_clear() {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10 | GPIO_PIN_7, GPIO_PIN_RESET);
}

__STATIC_INLINE void wait(__IO uint32_t us) {
  us *= (SystemCoreClock / 1000000) / 5;
  while (us--)
    ;
}

void anode_switch(uint8_t num) {
  switch (num) {
  case 0:
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    wait(DELAY1);
    all_clear();
    decoder_write(d);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    wait(DELAY2);
    break;
  case 1:
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    break;
  case 2:
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    wait(DELAY1);
    all_clear();
    decoder_write(c);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
    wait(DELAY2);
    break;
  case 3:
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    break;
  case 4:
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
    wait(DELAY1);
    all_clear();
    decoder_write(b);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    wait(DELAY2);
    break;
  case 5:
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    break;
  case 6:
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    wait(DELAY1);
    all_clear();
    decoder_write(a);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
    wait(DELAY2);
    break;
  case 7:
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
    break;
  }
}

void test() {
  uint32_t tmp_counter = 0;
  uint32_t tmp_value = 0;
  while (tmp_counter <= 15) {
    if ((HAL_GetTick() - _test_tick_prev) > TEST_FUNC_DELAY) {
      _test_tick_prev = GetTick_us();
      tmp_counter++;
      tmp_value = my_random(0, 9999);
    }

    a = tmp_value / 1000;
    b = (tmp_value / 100) % 10;
    c = (tmp_value / 10) % 10;
    d = tmp_value % 10;

    for (int i = 0; i < 8; i++) {
      anode_switch(i);
      wait(DELAY2);
    }
  }
}

void grb_to_pwm(uint32_t *grb_buf, uint16_t *out_buf) {
  for (int i = 0; i < RGB_NUM * 24 + RGB_OFFSET; i++) {
    if (i < (RGB_OFFSET / 2)) {
      out_buf[i] = PWM_MIN;
    } else if (i >= (RGB_OFFSET / 2) && i < RGB_NUM * 24 + (RGB_OFFSET / 2)) {
      _Bool current_bit = (grb_buf[((i - (RGB_OFFSET / 2)) / 24)] >>
                           (23 - ((i - (RGB_OFFSET / 2)) % 24))) %
                          2;
      if (current_bit) {
        out_buf[i] = PWM_HIGH;
      } else {
        out_buf[i] = PWM_LOW;
      }
    } else {
      out_buf[i] = PWM_MIN;
    }
  }
}

__STATIC_INLINE void bitbang_rgb_write(uint16_t *in_buf, int len) {
  __disable_irq();
  for (int i = 0; i < len; i++) {
    if (in_buf[i] == PWM_LOW) {
      GPIO_SET_PIN(GPIOA, GPIO_PIN_3);
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      GPIO_CLEAR_PIN(GPIOA, GPIO_PIN_3);
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
    } else if (in_buf[i] == PWM_HIGH) {
      GPIO_SET_PIN(GPIOA, GPIO_PIN_3);
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      GPIO_CLEAR_PIN(GPIOA, GPIO_PIN_3);
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
      __NOP();
    } else {
      GPIO_CLEAR_PIN(GPIOA, GPIO_PIN_3);
    }
  }
  __enable_irq();
}

void rgb_to_grb(uint32_t *rgb_buf, uint32_t *grb_buf) {
  for (int i = 0; i < RGB_NUM; i++) {
    grb_buf[i] = ((rgb_buf[i] & 0x00FF00) << 8) +
                 ((rgb_buf[i] & 0xFF0000) >> 8) + (rgb_buf[i] & 0x0000FF);
  }
}

void rgb_demo(RGBDemoStateMachine *states, uint32_t in_color,
              uint32_t grayscale_num, uint32_t grayscale_shift,
              uint32_t step_delay) {
  float tmp_r = (float)((in_color & 0xFF0000) >> 16);
  float tmp_g = (float)((in_color & 0x00FF00) >> 8);
  float tmp_b = (float)((in_color & 0x0000FF));
  if (!states->init) {
    states->color = grayscale_shift;
    states->top_point_delay_step = 0;
    states->init = true;
  }
  if ((GetTick_us() - states->get_tick_prev_us) > step_delay) {
    if (states->color < grayscale_num && states->top_point_delay_step == 0) {
      states->color++;
    } else if (states->color == grayscale_num &&
               states->top_point_delay_step < 4) {
      states->top_point_delay_step++;
    } else if (states->top_point_delay_step == 4 &&
               states->color > grayscale_shift) {
      states->color--;
    } else if (states->top_point_delay_step == 4 &&
               states->color == grayscale_shift) {
      states->init = false;
    }
  }
  uint32_t tmp_color_buf[RGB_NUM] = {
      0,
  };
  if (states->current_led_num < 4) {
    tmp_color_buf[states->current_led_num] =
        ((uint32_t)(tmp_r /
                    ((float)(grayscale_num) - (float)(states->color + 1)))
         << 16) +
        ((uint32_t)(tmp_g /
                    ((float)(grayscale_num) - (float)(states->color + 1)))
         << 8) +
        ((uint32_t)(tmp_b /
                    ((float)(grayscale_num) - (float)(states->color + 1))));
  }
  rgb_to_grb(tmp_color_buf, grb_colors_buf);
  grb_to_pwm(grb_colors_buf, dma_pwm_buff);
  bitbang_rgb_write(dma_pwm_buff, RGB_NUM * 24 + RGB_OFFSET);

  states->get_tick_prev_us = GetTick_us();
}

void reset_rgb_demo_states(RGBDemoStateMachine *states) {
  states->color = 0;
  states->top_point_delay_step = 0;
  states->get_tick_prev_us = 0;
  states->current_led_num = 0;
  states->init = false;
}

void set_array_with_color(uint32_t *rgb_buf, uint32_t color) {
  for (int i = 0; i < RGB_NUM; i++) {
    rgb_buf[i] = color;
  }
}

void SetReadOutProtection() {
  FLASH_OBProgramInitTypeDef Optbyte;

  __disable_irq();
  HAL_FLASHEx_OBGetConfig(&Optbyte);        // read out RDPLvL
  if (Optbyte.RDPLevel == OB_RDP_LEVEL_0) { //
    Optbyte.OptionType = OPTIONBYTE_RDP;    // select RDP optionbyte
    Optbyte.RDPLevel = OB_RDP_LEVEL_1;      // select RDP level 1
    HAL_FLASH_Unlock();                     // unlock Flash
    HAL_FLASH_OB_Unlock();                  // unlock Optionbytes
    HAL_FLASHEx_OBProgram(&Optbyte);        // set RDP=1
    HAL_FLASH_OB_Launch();                  // write OB to Flash and reset
    HAL_FLASH_OB_Lock();                    // Lock Optionbytes
    HAL_FLASH_Lock();                       // lock Flash
  }
  __enable_irq();
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
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
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
