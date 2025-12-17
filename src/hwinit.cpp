/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/desig.h>
#include "hwdefs.h"
#include "hwinit.h"
#include "stm32_loader.h"
#include "my_string.h"
#include "digio.h"
#include "params.h"

/**
* Start clocks of all needed peripherals
*/

uint16_t Tim3_Presc;
uint16_t Tim3_Period;
uint32_t Tim3_3_OC;
uint16_t Tim4_Presc;
uint16_t Tim4_Period;
uint32_t Tim4_1_OC;

void LoadValues()
{
	switch (Param::GetInt(Param::Tim3_Frequency))
	{
		case 1:
			Tim3_Presc = 71;
			Tim3_Period = 10000;
			break;
		case 2:
			Tim3_Presc = 63;
			Tim3_Period = 2250;
			break;
		case 3:
			Tim3_Presc = 31;
			Tim3_Period = 2250;
			break;
		case 4:
			Tim3_Presc = 9;
			Tim3_Period = 720;
			break;
		case 5:
			Tim3_Presc = 0;
			Tim3_Period = 720;
			break;
		default:
			Tim3_Presc = 63;
			Tim3_Period = 2250;
		break;
	}
	
	switch (Param::GetInt(Param::Tim4_Frequency))
	{
		case 1:
			Tim4_Presc = 71;
			Tim4_Period = 10000;
			break;
		case 2:
			Tim4_Presc = 63;
			Tim4_Period = 2250;
			break;
		case 3:
			Tim4_Presc = 31;
			Tim4_Period = 2250;
			break;
		case 4:
			Tim4_Presc = 9;
			Tim4_Period = 720;
			break;
		case 5:
			Tim4_Presc = 0;
			Tim4_Period = 720;
			break;
		default:
			Tim4_Presc = 63;
			Tim4_Period = 2250;
		break;
	}
	
	Param::SetInt(Param::PWM3CH3_DC, 0);
	if (Param::GetInt(Param::PWM3_CH3)) 
	   {
			int TIM3_3_DC = (100 - Param::GetInt(Param::Tim3_3_DC));
			Tim3_3_OC = TIM3_3_DC*Tim3_Period;
			Tim3_3_OC = Tim3_3_OC/100;
			Param::SetInt(Param::PWM3CH3_DC, Tim3_3_OC);
	   }
	
	Param::SetInt(Param::PWM4CH1_DC, 0);
	if (Param::GetInt(Param::PWM4_CH1)) 
	   {
			int TIM4_1_DC = (100 - Param::GetInt(Param::Tim4_1_DC));
			Tim4_1_OC = TIM4_1_DC*Tim4_Period;
			Tim4_1_OC = Tim4_1_OC/100;
			Param::SetInt(Param::PWM4CH1_DC, Tim4_1_OC);
	   }
}

void clock_setup(void)
{
   RCC_CLOCK_SETUP();

   //The reset value for PRIGROUP (=0) is not actually a defined
   //value. Explicitly set 16 preemtion priorities
   SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIGROUP_GROUP16_NOSUB;

   rcc_periph_clock_enable(RCC_GPIOA);
   rcc_periph_clock_enable(RCC_GPIOB);
   rcc_periph_clock_enable(RCC_GPIOC);
   rcc_periph_clock_enable(RCC_GPIOD);
   rcc_periph_clock_enable(RCC_USART3);
   rcc_periph_clock_enable(RCC_TIM2); //Scheduler
   rcc_periph_clock_enable(RCC_TIM3); //PWM outputs	
   rcc_periph_clock_enable(RCC_TIM4); //PWM outputs 
   rcc_periph_clock_enable(RCC_DMA1);  //ADC, Encoder and UART receive
   rcc_periph_clock_enable(RCC_ADC1);
   rcc_periph_clock_enable(RCC_CRC);
   rcc_periph_clock_enable(RCC_AFIO); //CAN
   rcc_periph_clock_enable(RCC_CAN1); //CAN
   rcc_periph_clock_enable(RCC_SPI1); //SPI1 for BATMAN!
}

/* Some pins should never be left floating at any time
 * Since the bootloader delays firmware startup by a few 100ms
 * We need to tell it which pins we want to initialize right
 * after startup
 */
void write_bootloader_pininit()
{
   uint32_t flashSize = desig_get_flash_size();
   uint32_t pindefAddr = FLASH_BASE + flashSize * 1024 - PINDEF_BLKNUM * PINDEF_BLKSIZE;
   const struct pincommands* flashCommands = (struct pincommands*)pindefAddr;

   struct pincommands commands;

   memset32((int*)&commands, 0, PINDEF_NUMWORDS);

   //!!! Customize this to match your project !!!
   //Here we specify that PC13 be initialized to ON
   //AND PB1 AND PB2 be initialized to OFF
   commands.pindef[0].port = GPIOC;
   commands.pindef[0].pin = GPIO13;
   commands.pindef[0].inout = PIN_OUT;
   commands.pindef[0].level = 1;
   commands.pindef[1].port = GPIOB;
   //commands.pindef[1].pin = GPIO1 | GPIO2;
   commands.pindef[1].pin = GPIO2;
   commands.pindef[1].inout = PIN_OUT;
   commands.pindef[1].level = 0;

   crc_reset();
   uint32_t crc = crc_calculate_block(((uint32_t*)&commands), PINDEF_NUMWORDS);
   commands.crc = crc;

   if (commands.crc != flashCommands->crc)
   {
      flash_unlock();
      flash_erase_page(pindefAddr);

      //Write flash including crc, therefor <=
      for (uint32_t idx = 0; idx <= PINDEF_NUMWORDS; idx++)
      {
         uint32_t* pData = ((uint32_t*)&commands) + idx;
         flash_program_word(pindefAddr + idx * sizeof(uint32_t), *pData);
      }
      flash_lock();
   }
}

/**
* Enable Timer refresh and break interrupts
*/
void nvic_setup(void)
{
   nvic_enable_irq(NVIC_TIM2_IRQ); //Scheduler
   nvic_set_priority(NVIC_TIM2_IRQ, 0xe << 4); //second lowest priority
}

void rtc_setup()
{
   //Base clock is HSE/128 = 8MHz/128 = 62.5kHz
   //62.5kHz / (624 + 1) = 100Hz
   rtc_auto_awake(RCC_HSE, 624); //10ms tick
   rtc_set_counter_val(0);
}

void spi1_setup()   //spi 1 used for BATMAN!
{
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7 | GPIO5);//MOSI , CLK
   gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);//MISO
   spi_reset(SPI1);
   spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
   spi_set_standard_mode(SPI1,1);//set mode 1
   spi_enable_software_slave_management(SPI1);
   spi_set_nss_high(SPI1);
   spi_enable(SPI1);
}

void tim3_setup()
{
   gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,GPIO0);
   timer_disable_counter(TIM3);
   //edge aligned PWM
   
   timer_set_alignment(TIM3, TIM_CR1_CMS_EDGE);
   timer_enable_preload(TIM3);
   /* PWM mode 1 and preload enable */
   timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
   timer_enable_oc_preload(TIM3, TIM_OC3);
   timer_set_oc_polarity_high(TIM3, TIM_OC3);
   timer_enable_oc_output(TIM3, TIM_OC3);
   timer_set_period(TIM3, Tim3_Period);
   timer_set_oc_value(TIM3, TIM_OC3, 0);
   Param::SetInt(Param::PWM3CH3, 0);
   if (Param::GetInt(Param::PWM3_CH3))
   {
	   timer_set_oc_value(TIM3, TIM_OC3, Tim3_3_OC);
	   Param::SetInt(Param::PWM3CH3, 1);
   }
   timer_generate_event(TIM3, TIM_EGR_UG);
   timer_set_prescaler(TIM3, Tim3_Presc);
   timer_enable_counter(TIM3);
}				 

void tim4_setup()
{
   gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_2_MHZ,GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,GPIO6);
   timer_disable_counter(TIM4);
   //edge aligned PWM
   timer_set_alignment(TIM4, TIM_CR1_CMS_EDGE);
   timer_enable_preload(TIM4);
   /* PWM mode 1 and preload enable */
   timer_set_oc_mode(TIM4, TIM_OC1, TIM_OCM_PWM1);
   timer_enable_oc_preload(TIM4, TIM_OC1);
   timer_set_oc_polarity_high(TIM4, TIM_OC1);
   timer_enable_oc_output(TIM4, TIM_OC1);
   timer_set_period(TIM4, Tim4_Period);
   timer_set_oc_value(TIM4, TIM_OC1, 0);
   Param::SetInt(Param::PWM4CH1, 0);
   if (Param::GetInt(Param::PWM4_CH1)) 
   {
	   timer_set_oc_value(TIM4, TIM_OC1, Tim4_1_OC);
	   Param::SetInt(Param::PWM4CH1, 1);
   }
   timer_generate_event(TIM4, TIM_EGR_UG);
   timer_set_prescaler(TIM4, Tim4_Presc);
   timer_enable_counter(TIM4);
}				 


