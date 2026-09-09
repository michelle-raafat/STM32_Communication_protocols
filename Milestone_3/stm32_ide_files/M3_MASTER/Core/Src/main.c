/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Milestone 3 - SPI Master
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * Send one character through USART1
 */
void UART_SendChar(char c)
{
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)&c,
                      1,
                      HAL_MAX_DELAY);
}


/*
 * Send a string through USART1
 */
void UART_SendString(char *str)
{
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)str,
                      strlen(str),
                      HAL_MAX_DELAY);
}


/*
 * Receive one character from USART1
 */
char UART_GetChar(void)
{
    char c;

    HAL_UART_Receive(&huart1,
                     (uint8_t *)&c,
                     1,
                     HAL_MAX_DELAY);

    return c;
}


/*
 * Convert ASCII Case ID to the required SPI ID.
 *
 * User enters:
 *
 * 1 -> 0x1001
 * 2 -> 0x1002
 * 3 -> 0x1003
 * 4 -> 0x1004
 * 5 -> 0x1005
 */
uint16_t GetCaseID(char input)
{
    switch (input)
    {
        case '1':
            return 0x1001;

        case '2':
            return 0x1002;

        case '3':
            return 0x1003;

        case '4':
            return 0x1004;

        case '5':
            return 0x1005;

        default:
            return 0;
    }
}


/*
 * Send Case ID to Slave.
 *
 * The Case ID is 16 bits and is sent MSB first.
 */
HAL_StatusTypeDef SendCaseID(uint16_t caseID)
{
    uint8_t txData[2];

    txData[0] = (uint8_t)((caseID >> 8) & 0xFF);
    txData[1] = (uint8_t)(caseID & 0xFF);

    /*
     * Select Slave
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_RESET);

    /*
     * Send 2 bytes
     */
    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1,
                         txData,
                         2,
                         HAL_MAX_DELAY);

    /*
     * Deselect Slave
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_SET);

    return status;
}


/*
 * Receive the 24-bit response from the Slave.
 *
 * SPI is full duplex, therefore the Master must transmit
 * dummy bytes to generate the clock.
 */
HAL_StatusTypeDef ReceiveResponse(uint8_t *rxData)
{
    uint8_t dummy[3] = {0x00, 0x00, 0x00};

    HAL_StatusTypeDef status;

    /*
     * Select Slave
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_RESET);

    /*
     * Clock 3 bytes from Slave.
     *
     * TransmitReceive is used because the Master
     * needs to generate clock pulses while receiving.
     */
    status = HAL_SPI_TransmitReceive(&hspi1,
                                     dummy,
                                     rxData,
                                     3,
                                     HAL_MAX_DELAY);

    /*
     * Deselect Slave
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_SET);

    return status;
}


/*
 * Decode the 24-bit message.
 *
 * Bits 23:12 = Voltage
 * Bits 11:0  = Wheel Speed
 */
void DecodeResponse(uint8_t *rxData,
                    uint16_t *voltage,
                    uint16_t *speed)
{
    uint32_t message;

    /*
     * Reconstruct 24-bit message
     */
    message = ((uint32_t)rxData[0] << 16) |
              ((uint32_t)rxData[1] << 8)  |
              ((uint32_t)rxData[2]);

    /*
     * Extract voltage
     */
    *voltage = (uint16_t)((message >> 12) & 0x0FFF);

    /*
     * Extract wheel speed
     */
    *speed = (uint16_t)(message & 0x0FFF);
}


/*
 * Print the received data.
 *
 * Voltage is stored as an integer:
 *
 * 123 = 12.3 V
 * 142 = 14.2 V
 */
void DisplayData(uint16_t voltage,
                 uint16_t speed)
{
    char buffer[100];

    UART_SendString("\r\n");

    sprintf(buffer,
            "Voltage : %d.%d V\r\n",
            voltage / 10,
            voltage % 10);

    UART_SendString(buffer);
    sprintf(buffer,
            "Wheel Speed: %d\r\n",
            speed);

    UART_SendString(buffer);

    UART_SendString("================================\r\n");
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */

    uint16_t caseID;
    uint16_t voltage;
    uint16_t speed;

    uint8_t rxData[3];

    char input;

    /* USER CODE END 1 */

    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    /* USER CODE BEGIN 2 */

    /*
     * Make sure Slave is not selected initially.
     *
     * CS = HIGH -> Slave disabled
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_SET);

    /*
     * Welcome message
     */
    UART_SendString("\r\n");
    UART_SendString("********************************\r\n");
    UART_SendString(" STM32 SPI Sensor Network\r\n");
    UART_SendString(" Master\r\n");
    UART_SendString("********************************\r\n");

    UART_SendString("Enter Case ID (1-5): ");

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        /*
         * ============================================================
         * STEP 1: Receive Case ID from Virtual Terminal
         * ============================================================
         */
        input = UART_GetChar();

        /*
         * Echo the entered character
         */
        UART_SendChar(input);

        UART_SendString("\r\n");


        /*
         * ============================================================
         * STEP 2: Convert input to Case ID
         * ============================================================
         */
        caseID = GetCaseID(input);


        /*
         * Check whether the Case ID is valid
         */
        if (caseID == 0)
        {
            UART_SendString("Invalid Case ID!\r\n");
            UART_SendString("Please enter a number from 1 to 5: ");

            continue;
        }


        /*
         * ============================================================
         * SPI CYCLE 1
         * ============================================================
         *
         * Send Case ID to Slave.
         *
         * Example:
         *
         * User enters:
         *      1
         *
         * Master sends:
         *      0x10
         *      0x01
         *
         * Slave receives:
         *      0x1001
         */
        if (SendCaseID(caseID) != HAL_OK)
        {
            UART_SendString("SPI Error while sending Case ID!\r\n");

            continue;
        }


        /*
         * Small delay between the two SPI transactions.
         *
         * This gives the Slave time to process the received
         * Case ID and prepare its 24-bit response.
         */
        HAL_Delay(1);


        /*
         * ============================================================
         * SPI CYCLE 2
         * ============================================================
         *
         * Receive 3 bytes from Slave.
         *
         * Master sends dummy bytes:
         *
         *      0x00 0x00 0x00
         *
         * Slave sends:
         *
         *      Voltage + Speed
         */
        if (ReceiveResponse(rxData) != HAL_OK)
        {
            UART_SendString("SPI Error while receiving response!\r\n");

            continue;
        }


        /*
         * ============================================================
         * STEP 3: Decode 24-bit response
         * ============================================================
         */
        DecodeResponse(rxData,
                       &voltage,
                       &speed);


        /*
         * ============================================================
         * STEP 4: Display result on Virtual Terminal
         * ============================================================
         */
        DisplayData(voltage,
                    speed);


        /*
         * Ask for another Case ID
         */
        UART_SendString("\r\nEnter Case ID (1-5): ");
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

    /*
     * Initializes the RCC Oscillators according to the specified
     * parameters in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;

    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_0) != HAL_OK)
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

    /*
     * Master mode
     */
    hspi1.Init.Mode = SPI_MODE_MASTER;

    /*
     * Full duplex
     */
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;

    /*
     * 8-bit SPI
     */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

    /*
     * Clock polarity = LOW
     */
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;

    /*
     * Clock phase = first edge
     */
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

    /*
     * Software NSS.
     *
     * PA0 is used as our manual CS.
     */
    hspi1.Init.NSS = SPI_NSS_SOFT;

    /*
     * SPI clock = APB2 clock / 2
     */
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;

    /*
     * MSB first
     */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;

    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    hspi1.Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
    /* USER CODE BEGIN USART1_Init 0 */

    /* USER CODE END USART1_Init 0 */

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */

    huart1.Instance = USART1;

    huart1.Init.BaudRate = 9600;

    huart1.Init.WordLength = UART_WORDLENGTH_8B;

    huart1.Init.StopBits = UART_STOPBITS_1;

    huart1.Init.Parity = UART_PARITY_NONE;

    huart1.Init.Mode = UART_MODE_TX_RX;

    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN USART1_Init 2 */

    /* USER CODE END USART1_Init 2 */
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

    /*
     * GPIO Ports Clock Enable
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();


    /*
     * PA0 initial state = HIGH
     *
     * HIGH = Slave not selected
     */
    HAL_GPIO_WritePin(GPIOA,
                      GPIO_PIN_0,
                      GPIO_PIN_SET);


    /*
     * Configure GPIO pin : PA0
     *
     * PA0 is our manual Chip Select / NSS signal.
     */
    GPIO_InitStruct.Pin = GPIO_PIN_0;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA,
                  &GPIO_InitStruct);


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

    /*
     * User can add implementation to report the file name and line number.
     */

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
