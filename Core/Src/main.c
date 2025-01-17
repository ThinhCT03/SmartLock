/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SH1106.h"
#include "fonts.h"
#include "stdio.h"
#include "bitmap.h"
#include "rcc522.h"
#include "string.h"

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16
uint8_t sNum[5];
volatile uint32_t systickCounter = 0;
volatile uint8_t rfidDetected = 0; // 0: không có thẻ, 1: có thẻ

// Các phím trên bàn phím
char keypad[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'E', '0', 'F', 'D'}
};

char key;
char enteredPassword[8] = {0};
char correctPassword[8] = "1234567";
int user = 0;

// Các chân hàng và cột của bàn phím
GPIO_TypeDef* rowPorts[4] = {GPIOB, GPIOB, GPIOB, GPIOB};
uint16_t rowPins[4] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13, GPIO_PIN_12};
GPIO_TypeDef* colPorts[4] = {GPIOA, GPIOA, GPIOA, GPIOA};
uint16_t colPins[4] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11};

//Khai báo máy trạng thái
   typedef enum {Do_RFID, Do_Keypad, Idle_State} ChooseState_t;
   static ChooseState_t ChooseState = Idle_State;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
void SysTick_Init(void);
char Keypad_Scan(void);
void CheckRFID(void);
void ChooseStateMachineUpdate(void);
void SysTick_Start(void);

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
  SysTick_Init();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  MFRC522_Init();
  SH1106_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  ChooseStateMachineUpdate();
    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}


void CheckRFID(void) {
    uchar TagType[2];
    if (MFRC522_Request(PICC_REQIDL, TagType) == MI_OK) {
        rfidDetected = 1; // Có thẻ trong phạm vi
    } else {
        rfidDetected = 0; // Không có thẻ
    }
}



//Hàm xây dựng ngắt systick
void SysTick_Init(void)
{
    // Tính giá trị LOAD cho 1ms
    uint32_t ticks = SystemCoreClock / 1000; // 1ms
    if (ticks > SysTick_LOAD_RELOAD_Msk)
    {
        // Giá trị LOAD không hợp lệ
        Error_Handler();
    }

    // Ghi giá trị vào thanh ghi LOAD
    SysTick->LOAD = ticks - 1;

    // Reset giá trị hiện tại của bộ đếm
    SysTick->VAL = 0;

    // Cấu hình SysTick: nguồn clock, bật ngắt, và kích hoạt
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | // Sử dụng clock hệ thống
                    SysTick_CTRL_TICKINT_Msk;  // Bật ngắt SysTick
}

void SysTick_Start(void)
{
    // Kích hoạt bộ đếm SysTick
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;  // Bật bộ đếm SysTick
}


// Hàm xử lý ngắt SysTick (được gọi mỗi 1ms)
void SysTick_Handler(void)
{
    // Xử lý công việc cần thực hiện trong ngắt SysTick
    // Ví dụ: tăng biến đếm thời gian
    systickCounter++;
}




/**
  * @brief System Clock Configuration
  * @retval None
  */

void ChooseStateMachineUpdate(void)
{
	switch (ChooseState)
	{
	case Do_RFID:
		  status = MFRC522_Request(PICC_REQIDL, str);
		  status = MFRC522_Anticoll(str);
		  memcpy(sNum, str, 5);
		  HAL_Delay(100);
		  if((str[0]==147) && (str[1]==32) && (str[2]==210) && (str[3]==38) && (str[4]==212) )
		   {
			 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, SET);
			 systickCounter = 0;
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, RESET); //LED xanh báo thẻ đúng
             SH1106_Fill(SH1106_COLOR_BLACK);
             SH1106_GotoXY(10, 10);
             SH1106_Puts("Card Valid!", &Font_11x18, SH1106_COLOR_WHITE);
             SH1106_UpdateScreen();
             HAL_Delay(1000);
		     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, SET);
		     }
		   else if((str[0]==147) && (str[1]==32) && (str[2]==115) && (str[3]==110) && (str[4]==207) )
		    {
			 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, SET);
			 systickCounter = 0;
			 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, RESET); //LED xanh báo thẻ đúng
             SH1106_Fill(SH1106_COLOR_BLACK);
             SH1106_GotoXY(10, 10);
             SH1106_Puts("Card is Valid!", &Font_11x18, SH1106_COLOR_WHITE);
             SH1106_UpdateScreen();
		     HAL_Delay(1000);
		     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, SET);
		   }
		   else if (rfidDetected)
		   {
			 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, SET);
		     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, RESET); //LED đỏ báo thẻ sai
             SH1106_Fill(SH1106_COLOR_BLACK);
             SH1106_GotoXY(10, 10);
             SH1106_Puts("Card is not Valid!", &Font_11x18, SH1106_COLOR_WHITE);
             SH1106_UpdateScreen();
		     HAL_Delay(1000);
		     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, SET);
		  }
		   else
		   {
			   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, RESET); //Led vàng báo nguồn
		   }
		  break;

	case Do_Keypad:
 	      key = Keypad_Scan();
	       if (key) {
	           if (user < 7) {
	               enteredPassword[user++] = key;
	               enteredPassword[user] = '\0'; // Cập nhật kết thúc chuỗi

	               // Hiển thị chuỗi nhập trên OLED
	               SH1106_Fill(SH1106_COLOR_BLACK);
	               SH1106_GotoXY(10, 10);
	               SH1106_Puts(enteredPassword, &Font_11x18, SH1106_COLOR_WHITE);
	               SH1106_UpdateScreen();
	               HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, RESET); //Led vàng báo nguồn
//	               HAL_Delay(200);

	           } else if (user == 6) {
	               // Kiểm tra mật khẩu
	               if (strcmp(enteredPassword, correctPassword) == 0) {
	            	   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, SET);
	            	   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, RESET); //LED xanh báo thẻ đúng
	            	   SH1106_Fill(SH1106_COLOR_BLACK);
	                   SH1106_GotoXY(10, 10);
	                   SH1106_Puts("Right Password", &Font_11x18, SH1106_COLOR_WHITE);
	                   SH1106_UpdateScreen();
	                   HAL_Delay(1000);
	                   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, SET);
	               } else {
	            	   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, SET);
	            	   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, RESET); //LED đỏ báo thẻ sai
	                   SH1106_Fill(SH1106_COLOR_BLACK);
	                   SH1106_GotoXY(10, 10);
	                   SH1106_Puts("Wrong Password", &Font_11x18, SH1106_COLOR_WHITE);
	                   SH1106_UpdateScreen();
	                   HAL_Delay(1000);
	                   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, SET);
	               }

	               // Reset trạng thái nhập mật khẩu
	               user = 0;
	               memset(enteredPassword, 0, sizeof(enteredPassword));

	               HAL_Delay(1000); // Đợi trước khi tiếp tục

	               // Xóa màn hình
	               SH1106_Fill(SH1106_COLOR_BLACK);
	               SH1106_UpdateScreen();
	           }
	       }
	       break;

	case Idle_State:
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, RESET); //Led vàng báo nguồn
		  SH1106_Fill(SH1106_COLOR_BLACK);
		  SH1106_DrawBitmap(2, 0, LOGO_BW_map, 128, 49 , 1);  // 132x64, so leave 2 from both sides, draw from 0,0
		  SH1106_UpdateScreen();
		  break;
	}



	switch (ChooseState)
	{
	case Do_RFID:
		  SysTick_Start(); //Start timer
		  CheckRFID();
	      key = Keypad_Scan();
	      if (key && systickCounter < 3000)
	      {
	    	  systickCounter = 0;//reset systick nêú có phím đang nhập
	    	  ChooseState = Do_Keypad;
	      }
	      else if (systickCounter >= 3000) // systickCounter >= 3000
		  {
	    	  ChooseState = Idle_State;
		  }
	      else
	      {
	    	  if (rfidDetected){
	    		  systickCounter = 0;//reset systick nêú có thẻ đang quẹt
	    	  }
	    	  ChooseState = Do_RFID;
	      }
	      break;

	case Do_Keypad:
		  SysTick_Start(); //Start timer
		  key = Keypad_Scan();
	      CheckRFID();
	      if (rfidDetected && systickCounter < 3000)
	      {
	    	  ChooseState = Do_RFID;
	    	  systickCounter = 0; //reset systick nêú có thẻ đang được quẹt
	      }
	      else if (systickCounter >= 3000) // systickCounter >= 3000
	      {
	    	  ChooseState = Idle_State;
	      }
	      else
	      {
	    	  if (key){
	    		  systickCounter = 0;//reset systick nêú có phím đang nhập
	    	  }
	    	  ChooseState = Do_Keypad;
	      }
	      break;

	case Idle_State:
		key = Keypad_Scan();
		CheckRFID();
		if (key)
		{
			ChooseState = Do_Keypad;
		}
		else if (rfidDetected)
		{
			ChooseState = Do_RFID;
		}
		else
		{
			ChooseState = Idle_State;
		}
		break;

	}
}



void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB12 PB13 PB14
                           PB15 PB4 PB5 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 PA11 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
char Keypad_Scan(void) {
    for (int row = 0; row < 4; row++) {
        // Đặt tất cả hàng LOW trước khi đặt hàng hiện tại lên HIGH
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_RESET);
        }

        // Đặt hàng hiện tại lên HIGH
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_SET);

        // Kiểm tra từng cột
        for (int col = 0; col < 4; col++) {
        	for (uint8_t i = 0; i < 100; i++){ //Đọc 100 lần để chống rung phím
            if (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_SET) {
            	//HAL_Delay(50);
                // Chờ cho đến khi nút được nhả ra
                while (HAL_GPIO_ReadPin(colPorts[col], colPins[col]) == GPIO_PIN_SET);

                // Khôi phục hàng về LOW trước khi thoát
                HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_RESET);

                return keypad[row][col];
            }
        	}
        }

        // Khôi phục hàng về HIGH
        HAL_GPIO_WritePin(rowPorts[row], rowPins[row], GPIO_PIN_SET);
    }

    return 0; // Không có nút nào được nhấn
}
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
