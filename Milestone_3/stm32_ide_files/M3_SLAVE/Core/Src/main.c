/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Milestone 3 - SPI Slave
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN 0 */

/*
 * Case ID and corresponding data
 *
 * Voltage is stored as:
 * 12.3 V -> 123
 * 14.2 V -> 142
 * 13.8 V -> 138
 * 11.5 V -> 115
 * 10.9 V -> 109
 *
 * The voltage and speed are packed into a 24-bit message:
 *
 * Bits 23:12 -> Voltage
 * Bits 11:0  -> Wheel Speed
 */
typedef struct
{
    uint16_t id;
    uint16_t voltage;
    uint16_t speed;
} CaseData;

CaseData cases[5] =
{
    {0x1001, 123, 200},
    {0x1002, 142, 150},
    {0x1003, 138, 100},
    {0x1004, 115, 220},
    {0x1005, 109, 280}
};


/*
 * Encode voltage and speed into a 24-bit value
 */
uint32_t EncodeData(uint16_t voltage, uint16_t speed)
{
    uint32_t message;

    message = ((uint32_t)(voltage & 0x0FFF) << 12);
    message |= (speed & 0x0FFF);

    return message;
}


/*
 * Find the data corresponding to the received Case ID
 */
uint32_t GetResponse(uint16_t id)
{
    uint8_t i;

    for (i = 0; i < 5; i++)
    {
        if (cases[i].id == id)
        {
            return EncodeData(cases[i].voltage,
                              cases[i].speed);
        }
    }

    /*
     * Invalid Case ID
     */
    return 0;
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    uint8_t rxID[2];
    uint8_t txData[3];

    uint16_t receivedID;
    uint32_t response;

    /* USER CODE END 2 */


    /* Infinite loop */
    while (1)
    {
        /*
         * ============================================================
         * SPI CYCLE 1
         * ============================================================
         *
         * Receive the 16-bit Case ID from the Master.
         *
         * Example:
         *
         * Master sends:
         * 0x1001
         *
         * rxID[0] = 0x10
         * rxID[1] = 0x01
         */

        if (HAL_SPI_Receive(&hspi1,
                            rxID,
                            2,
                            HAL_MAX_DELAY) == HAL_OK)
        {
            /*
             * Convert two bytes into a 16-bit Case ID
             *
             * MSB first:
             *
             * receivedID = [byte0][byte1]
             */
            receivedID = ((uint16_t)rxID[0] << 8) |
                          rxID[1];


            /*
             * Find the corresponding voltage and speed
             */
            response = GetResponse(receivedID);


            /*
             * ========================================================
             * Convert 24-bit response into 3 bytes
             * ========================================================
             *
             * Example:
             *
             * response = 0x07B00C8
             *
             * txData[0] = upper 8 bits
             * txData[1] = middle 8 bits
             * txData[2] = lower 8 bits
             */
            txData[0] = (uint8_t)((response >> 16) & 0xFF);
            txData[1] = (uint8_t)((response >> 8)  & 0xFF);
            txData[2] = (uint8_t)(response & 0xFF);


            /*
             * ========================================================
             * SPI CYCLE 2
             * ========================================================
             *
             * Send the 24-bit response to the Master.
             */
            HAL_SPI_Transmit(&hspi1,
                             txData,
                             3,
                             HAL_MAX_DELAY);
        }
    }
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
     * Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
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

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
    /* SPI1 parameter configuration*/

    hspi1.Instance = SPI1;

    /*
     * Slave mode
     */
    hspi1.Init.Mode = SPI_MODE_SLAVE;

    /*
     * Full duplex
     */
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;

    /*
     * 8-bit SPI transfers
     */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

    /*
     * Clock polarity LOW
     */
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;

    /*
     * Clock phase = first edge
     */
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

    /*
     * IMPORTANT:
     *
     * Slave uses hardware NSS.
     *
     * PA4 = SPI1_NSS
     */
    hspi1.Init.NSS = SPI_NSS_HARD_INPUT;

    /*
     * Most significant bit first
     */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    /*
     * Enable GPIOA clock
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
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
    /*
     * User can add implementation to report the file name and line number.
     */
}

#endif /* USE_FULL_ASSERT *//* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Milestone 3 - SPI Slave
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN 0 */

/*
 * Case ID and corresponding data
 *
 * Voltage is stored as:
 * 12.3 V -> 123
 * 14.2 V -> 142
 * 13.8 V -> 138
 * 11.5 V -> 115
 * 10.9 V -> 109
 *
 * The voltage and speed are packed into a 24-bit message:
 *
 * Bits 23:12 -> Voltage
 * Bits 11:0  -> Wheel Speed
 */
typedef struct
{
    uint16_t id;
    uint16_t voltage;
    uint16_t speed;
} CaseData;

CaseData cases[5] =
{
    {0x1001, 123, 200},
    {0x1002, 142, 150},
    {0x1003, 138, 100},
    {0x1004, 115, 220},
    {0x1005, 109, 280}
};


/*
 * Encode voltage and speed into a 24-bit value
 */
uint32_t EncodeData(uint16_t voltage, uint16_t speed)
{
    uint32_t message;

    message = ((uint32_t)(voltage & 0x0FFF) << 12);
    message |= (speed & 0x0FFF);

    return message;
}


/*
 * Find the data corresponding to the received Case ID
 */
uint32_t GetResponse(uint16_t id)
{
    uint8_t i;

    for (i = 0; i < 5; i++)
    {
        if (cases[i].id == id)
        {
            return EncodeData(cases[i].voltage,
                              cases[i].speed);
        }
    }

    /*
     * Invalid Case ID
     */
    return 0;
}

/* USER CODE END 0 */


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration------------------------------------------------------*/

    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */

    uint8_t rxID[2];
    uint8_t txData[3];

    uint16_t receivedID;
    uint32_t response;

    /* USER CODE END 2 */


    /* Infinite loop */
    while (1)
    {
        /*
         * ============================================================
         * SPI CYCLE 1
         * ============================================================
         *
         * Receive the 16-bit Case ID from the Master.
         *
         * Example:
         *
         * Master sends:
         * 0x1001
         *
         * rxID[0] = 0x10
         * rxID[1] = 0x01
         */

        if (HAL_SPI_Receive(&hspi1,
                            rxID,
                            2,
                            HAL_MAX_DELAY) == HAL_OK)
        {
            /*
             * Convert two bytes into a 16-bit Case ID
             *
             * MSB first:
             *
             * receivedID = [byte0][byte1]
             */
            receivedID = ((uint16_t)rxID[0] << 8) |
                          rxID[1];


            /*
             * Find the corresponding voltage and speed
             */
            response = GetResponse(receivedID);


            /*
             * ========================================================
             * Convert 24-bit response into 3 bytes
             * ========================================================
             *
             * Example:
             *
             * response = 0x07B00C8
             *
             * txData[0] = upper 8 bits
             * txData[1] = middle 8 bits
             * txData[2] = lower 8 bits
             */
            txData[0] = (uint8_t)((response >> 16) & 0xFF);
            txData[1] = (uint8_t)((response >> 8)  & 0xFF);
            txData[2] = (uint8_t)(response & 0xFF);


            /*
             * ========================================================
             * SPI CYCLE 2
             * ========================================================
             *
             * Send the 24-bit response to the Master.
             */
            HAL_SPI_Transmit(&hspi1,
                             txData,
                             3,
                             HAL_MAX_DELAY);
        }
    }
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
     * Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
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

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
    /* SPI1 parameter configuration*/

    hspi1.Instance = SPI1;

    /*
     * Slave mode
     */
    hspi1.Init.Mode = SPI_MODE_SLAVE;

    /*
     * Full duplex
     */
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;

    /*
     * 8-bit SPI transfers
     */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

    /*
     * Clock polarity LOW
     */
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;

    /*
     * Clock phase = first edge
     */
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

    /*
     * IMPORTANT:
     *
     * Slave uses hardware NSS.
     *
     * PA4 = SPI1_NSS
     */
    hspi1.Init.NSS = SPI_NSS_HARD_INPUT;

    /*
     * Most significant bit first
     */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;

    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    /*
     * Enable GPIOA clock
     */
    __HAL_RCC_GPIOA_CLK_ENABLE();
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
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
    /*
     * User can add implementation to report the file name and line number.
     */
}

#endif /* USE_FULL_ASSERT */
