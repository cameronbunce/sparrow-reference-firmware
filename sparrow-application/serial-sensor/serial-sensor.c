// Copyright 2023 Cameron Bunce
// Licensed alike with the work from Blues that makes this all possible
// The mistakes contained herein are mine

#include "serial-sensor.h"

/*
| This Application is designed to provide serial extensions to 
| an offboard processor to speed application testing
| and to enable Edge Impulse models on higher-horsepower sister microcontrollers
| 
| EXPECT THIS TO BREAK - it is not finished
| EXPECT THIS TO CHANGE - implemtation is not final
| 
| Enabling this application will check to see if a device connected on the 
| Tx/Rx pins responds to a "Hello" message
| 
| Polling intervals "Poll" the sister microcontroller for the latest sensor reading
| Responses for which include, at an interval determined by the other device
| the assessment of the latest data through the Edge Impulse model
*/

// memory
#include <malloc.h>

// Blues headers
#include <framework.h>
#include <note.h>

// If TRUE we have a device attached that is doing the big number crunching
#define SERIAL_SENSOR_MODE          false

#define REQUESTID_TEMPLATE          42

// This would be better not set at compile time, 
// but rather set at initialization 
// or in the response from the sister device
// yielding "*#DS18B20.qo" or "*#edgeimpulse.qo" 
// as indicated in the response
#define APPLICATION_NOTEFILE        "*#serial.qo" 

typedef struct applicationContext {
    /// @brief collection of things we need to remember as an application
    bool templateRegistered;
    bool done;
} applicationContext;
UART_HandleTypeDef huart2;


// Forwards
static void addSerialNote(applicationContext * ctx, bool immediate);
static const char * serialStateName(int state);
static bool registerNotefileTemplate (void);

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

// Scheduled App One-Time Init
bool serialsensorInit(void)
{
    APP_PRINTF("Serial Sensor: Initializing application \r\n");
    bool result = false;

    // Allocate and warm up application context
    applicationContext *ctx = (applicationContext *)malloc(sizeof(applicationContext));
    ctx->done = false;
    ctx->templateRegistered = false;

    // wrangle the serial conection and say hello
    //...
    // we need to know from the attached device what it will be telling us
    // we need to know the sensor name(s) and data types
    // for instance:
    // ESP32        TSTRINGV for system messages
    // DS18B20      TFLOAT32
    // Inference    TSTRINGV
    // Probably as JSON from the sister device
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    // Register the app
    schedAppConfig config = {
        .name = "serial-sensor",
        .activationPeriodSecs = 60 * 100,
        .pollPeriodSecs = 15,
        .activateFn = serialsensorActivate,
        .interruptFn = serialsensorISR,
        .pollFn = serialsensorPoll,
        .responseFn = serialsensorResponse,
        .appContext = ctx,
    };

    // Regitration was successful if the application identifier
    // is greater than or equal to zero
    result = (0 <= schedRegisterApp(&config));

    return result;
}

// Interrupt handler, maybe
// void serial-sensorISR()

static bool registerNotefileTemplate(void)
{
    APP_PRINTF("Serial Sensor: template registration request");
    
    // Create the request
    J *req = NoteNewRequest("note.template");
    if (req == NULL) {
        return false;
    }
    
    // Create the body
    J *body = JCreateObject();
    if (body == NULL) {
        JDelete(req);
        return false;
    }
    // Fill-in request parameters.  Note that in order to minimize
    // the size of the over-the-air JSON we're using a special format
    // for the "file" parameter implemented by the gateway, in which
    // a "file" parameter beginning with * will have that character
    // substituted with the textified Sparrow node address.
    JAddStringToObject(req, "file", APPLICATION_NOTEFILE);

    // Add an ID to the request, which will be echo'ed
    // back in the response by the notecard itself.  This
    // helps us to identify the asynchronous response
    // without needing to have an additional state.
    JAddNumberToObject(req, "id", REQUESTID_TEMPLATE);

    // Create the body
    J *body = JAddObjectToObject(req, "body");
    if(body == NULL){
        JDelete(req);
        return false;
    }

    // Fill in the body template with message from sister device
    // this needs to be determined at init
    // need to make sure gateway support multiple templates from a single application
    // otherwise we'll just pretend to be an application for each "type" of response we get
    // from the sister device
    JAddObjectToObject(body, ~serialsensortype, type);

    // Send request to the gateway
    noteSendToGatewayAsync(req, true);

}

// Gateway response handler
void serialsensorResponse(int appID, J *rsp, void *appContext)
{
    APP_PRINTF("Serial Sensor: Entered application callback function: serialsensorResponse\r\n\tappId: %d", appID);

    applicationContext *ctx = appContext;

    // See if there's an error
    char *err = JGetString(rsp, "err");
    if (err[0] != '\0') {
        APP_PRINTF("Serial Sensor: gateway returned error: %s\r\n", err);
        return;
    }

    // Identify specific response(s)
    // We probably need to log the appid to map templates to sensor types
    switch (JGetInt(rsp, "id")) {
    case REQUESTID_TEMPLATE:
        ctx->templateRegistered = true;
        APP_PRINTF("Serial Sensor: SUCCESSFUL template registration\r\n");
        break;
    default:
        APP_PRINTF("Serial Sensor: received unexpected response\r\n");
    }

}

static void addSerialNote(applicationContext * ctx, bool immediate)
{
    APP_PRINTF("Serial Sensor: generating note\r\n");

    // Create the request
    J *req = NoteNewRequest("note.add");
    if (req == NULL){
        return;
    }

    // If Immediate, sync now
    if(immediate){
        JAddBoolToObject(req, "sync", true);
    }

    // Set the target notefile
    // Ideally this would take a sensor parameter from the sister device
    JAddStringToObject(req, "file", APPLICATION_NOTEFILE);
    //JAddStringToObject(req, "file", ~serialstruct.sensor);

    // Create the Body
    J *body = JAddObjectToObject(req, "body");
    if (body == NULL){
        JDelete(req);
        return;
    }

    // Fill in the body
    JAddNumberToObject(body, );
}


/**
  * @brief System Clock Configuration
  * @retval None
  * yea, these are boiler-plate that I copied from STM32IDE to figure out how to do this.
  * I'm not sure how well they'll work in this application
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    APP_PRINTF("Serial Sensor: Something went wrong configuring the Oscillator.\r\n");
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    APP_PRINTF("Serial Sensor: Something went wrong configuring the clock.\r\n");
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

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

#ifdef  USE_FULL_ASSERT
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
