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
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/desig.h>
#include "stm32_can.h"
#include "canmap.h"
#include "cansdo.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "terminalcommands.h"
#include "my_string.h"
#include "digipot.h"
#include "utils.h"
#define PRINT_JSON 0

extern "C" void __cxa_pure_virtual()
{
    while (1);
}

static Stm32Scheduler* scheduler;
static CanHardware* can;
static CanMap* canMap;
static CanSdo* canSdo;

static int counter1 = 0;
int CAN_ON = 0;
int Mot_Temp = 0;
int Ac_Req = 0;
int HTR_Req = 0;

    // LUT — generated from your exact Steinhart-Hart coefficients
static const int32_t lut_R[] =
	{
	 13380,  //   0°C
	  8480,  //   5°C
	  5723,  //  10°C
	  4039,  //  15°C
	  2948,  //  20°C
	  2211,  //  25°C
	  1696,  //  30°C
	  1325,  //  35°C
	  1052,  //  40°C
	   847,  //  45°C
	   690,  //  50°C
	   569,  //  55°C
	   473,  //  60°C
	   397,  //  65°C
	   336,  //  70°C
	   287,  //  75°C
	   246,  //  80°C
	   213,  //  85°C
	   185,  //  90°C
	   161,  //  95°C
	   142,  // 100°C
	   125,  // 105°C
	   111,  // 110°C
		99,  // 115°C
		88,  // 120°C
	};

#define LUT_SIZE  25
#define TEMP_MIN   0    // °C
#define STEP       5    // °C per entry

static void Ms10Task(void)
{
    //Set timestamp of error message
    ErrorMessage::SetTime(rtc_get_counter_val());
	Param::SetInt(Param::MODE, (Param::GetInt(Param::Mode)));
	switch (Param::GetInt(Param::PUMP))
	{
		case 1:
			{
				Param::SetInt(Param::PWMPUMP, 1);
				Param::SetInt(Param::Pump_DC, Param::GetInt(Param::PUMP_DC));
				switch (Param::GetInt(Param::PUMP_Frequency))
					{
						case 0:
							CH1Low1Hz();
							break;
						case 1:
							CH1Low2Hz();
							break;
						case 2:
							CH1Low10Hz();
							break;
					default:
						//Handle general parameter changes here. Add paramNum labels for handling specific parameters
					break;
					}
			}
			break;
		default:
			Param::SetInt(Param::PWMPUMP, 0);
			Param::SetInt(Param::PUMP_DC, 0);
			DigIo::LOW_CH1.Clear();
		break;
	}
}

void Temp_Read()
{
  float Res = AnaIn::GP_analog2.Get();
  Param::SetInt(Param::GP2, Res);
  Res = (2190*Res) / (4095 - Res);			//steinhart equation to estimate temperature value at any resistance from curve of thermistor sensor
  
  for (int32_t i = 0; i < LUT_SIZE - 1; i++)
    {
        if (Res >= lut_R[i + 1])
        {
            int32_t span   = lut_R[i] - lut_R[i + 1];
            int32_t offset = (span > 0) ? ((STEP * 10) * (lut_R[i] - Res)) / span : 0;
            int32_t tenths = (TEMP_MIN * 10) + (i * STEP * 10) + offset;
			if (tenths <= 0) tenths = 0;
			if (tenths > 1200) tenths = 120;
            // Convert tenths back to float for SetFloat e.g. 235 → 23.5
            Param::SetFloat(Param::Temp_Sensor, tenths / 10.0f);
			return;   // ← critical
        }
    }
}

//sample 100ms task
static void Ms100Task(void)
{
	int opmode = Param::GetInt(Param::MODE);
	DigIo::led_out.Toggle();
    iwdg_reset();
    float cpuLoad = scheduler->GetCpuLoad();
    Param::SetFloat(Param::CPU_LOAD, cpuLoad / 10);
	int GP1_IN = AnaIn::GP_analog1.Get();
	Param::SetInt(Param::GP1, GP1_IN);
	Temp_Read();
	if (Mot_Temp <= 10) Mot_Temp = 10;
	if (Mot_Temp > 100) Mot_Temp = 100;
	Mot_Temp = Param::GetInt(Param::CAN_MotTemp);
	if(opmode==MOD_CHARGE || opmode==MOD_RUN)
	{	
		Param::SetInt(Param::PUMP, 1);
		Param::SetInt(Param::PUMP_Frequency,1);
		int PumpDC = utils::change(Mot_Temp, 25, 55, 44, 80);
		if (PumpDC <= 44) PumpDC = 44;
		if (PumpDC > 80) PumpDC = 80;
		Param::SetInt(Param::PUMP_DC, PumpDC);
		Param::SetInt(Param::FAN_Frequency,1);
		int FanDC = utils::change(Mot_Temp, 40, 60, 45, 98);
		if (FanDC <= 45) FanDC = 45;
		if (FanDC > 98) FanDC = 98;
		if (Param::GetInt(Param::CAN_AC)) FanDC = 98;
		if ((Mot_Temp >= 40) || (Param::GetInt(Param::CAN_AC))) Param::SetInt(Param::FAN_DC, FanDC);
		else Param::SetInt(Param::FAN_DC, 15);
		if (Param::GetInt(Param::CAN_HTR)) 
		{
			DigIo::Out1.Set();
			Param::SetInt(Param::Out1, 1);
			DigIo::Out2.Set();
			Param::SetInt(Param::Out2, 1);
		}
		else
		{
			DigIo::Out1.Clear();
			Param::SetInt(Param::Out1, 0);
			DigIo::Out2.Clear();
			Param::SetInt(Param::Out2, 0);
		}
	}
	
	if(opmode==0)
	{
		Param::SetInt(Param::PUMP, 1);
		Param::SetInt(Param::PUMP_Frequency,1);
		Param::SetInt(Param::PUMP_DC, 10);
		Param::SetInt(Param::FAN, 1);
		Param::SetInt(Param::FAN_Frequency,1);
		Param::SetInt(Param::FAN_DC, 15);
		DigIo::Out1.Clear();
		Param::SetInt(Param::Out1, 0);
		DigIo::Out2.Clear();
		Param::SetInt(Param::Out2, 0);
	}
		

	canMap->SendAll();
	Can_Tasks();
	LoadValues();
	tim4_setup();
	UpdateSOC();
}

static void Ms200Task(void)
{
	DigiPot::SetPot1Step();
	DigiPot::SetPot2Step();
	DigiPot::SetPot3Step();
	DigiPot::SetPot4Step();
	CAN_ON = 0;
	Param::SetInt(Param::CAN_MotTemp, 0);
	Param::SetInt(Param::CAN_AC, 0);
	Param::SetInt(Param::CAN_HTR, 0);
}


void Can_Tasks()
{	
	
}

void UpdateSOC()
{
	int SOC = Param::GetInt(Param::SOC);
	int PotValue = utils::change(SOC, 0, 100, 2, 138);
	Param::SetInt(Param::Pot1, PotValue);
	Param::SetInt(Param::Pot2, PotValue);
	Param::SetInt(Param::Pot3, PotValue);
	Param::SetInt(Param::Pot4, PotValue);
}

void CH1Low1Hz()
{
int DC = Param::GetInt(Param::PUMP_DC);
float onSteps = (DC * 100) / 100;

  if (counter1 < onSteps) 
	  {
		DigIo::LOW_CH1.Set();
	  } 
	  else 
	  {
		DigIo::LOW_CH1.Clear();
	  }
	
	counter1++;
  
  if (counter1 >= 100) 
	  {
		counter1 = 0; // Restart PWM cycle every 500 ms
	  }
}

void CH1Low2Hz()
{
int DC = Param::GetInt(Param::PUMP_DC);
float onSteps = (DC * 50) / 100;

  if (counter1 < onSteps) 
	  {
		DigIo::LOW_CH1.Set();
	  } 
	  else 
	  {
		DigIo::LOW_CH1.Clear();
	  }
	
	counter1++;
  
  if (counter1 >= 50) 
	  {
		counter1 = 0; // Restart PWM cycle every 500 ms
	  }
}

void CH1Low10Hz()
{
int DC = Param::GetInt(Param::PUMP_DC);
float onSteps = (DC * 10) / 100;

  if (counter1 < onSteps) 
	  {
		DigIo::LOW_CH1.Set();
	  } 
	  else 
	  {
		DigIo::LOW_CH1.Clear();
	  }
	
	counter1++;
  
  if (counter1 >= 10) 
	  {
		counter1 = 0; // Restart PWM cycle every 500 ms
	  }
}

	
void DecodeCAN(int id, uint32_t* data)
{
	uint8_t* bytes = (uint8_t*)data;
	switch (id)
	{
		case 0x500:
			Param::SetInt(Param::Mode, bytes[0]);
			Param::SetInt(Param::SOC, bytes[1]);
			break;
		case 0x501:
			Param::SetInt(Param::CAN_MotTemp, bytes[0]);
			break;
		case 0x502:
			Param::SetInt(Param::CAN_AC, bytes[0]);
			break;
		case 0x503:
			Param::SetInt(Param::CAN_HTR, bytes[0]);
			break;
		default:
			break;
	}
			
}



static void SetCanFilters()
{
	can->RegisterUserMessage(0x605); //Can SDO
	can->RegisterUserMessage(0x500); //OI Control Message
	can->RegisterUserMessage(0x501); //OI Control Message
	can->RegisterUserMessage(0x502); //OI Control Message
	can->RegisterUserMessage(0x503); //OI Control Message
	
}
	
static bool CanCallback(uint32_t id, uint32_t data[2], uint8_t dlc) //This is where we go when a defined CAN message is received.
{
    dlc = dlc;
	if (Param::GetInt(Param::CanCtrl)) DecodeCAN(id,data);		
	return false;
}

/** This function is called when the user changes a parameter */


void Param::Change(Param::PARAM_NUM paramNum)
{
    switch (paramNum)
    {
		case Param::NodeId:
			canSdo->SetNodeId(Param::GetInt(Param::NodeId));
			break;
		case Param::FAN_Frequency:
		case Param::FAN:
		case Param::FAN_DC:
			LoadValues();
			tim4_setup();
			break;
		case Param::CanCtrl:
			SetCanFilters();
			break;
	default:
        //Handle general parameter changes here. Add paramNum labels for handling specific parameters
		
        break;
    }
}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim2_isr(void)
{
    scheduler->Run();
}

extern "C" int main(void)
{
    extern const TERM_CMD termCmds[];

    clock_setup(); //Must always come first
    rtc_setup();
    ANA_IN_CONFIGURE(ANA_IN_LIST);
    DIG_IO_CONFIGURE(DIG_IO_LIST);
	AnaIn::Start(); //Starts background ADC conversion via DMA
    write_bootloader_pininit(); //Instructs boot loader to initialize certain pins
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_CAN1_REMAP_PORTB);//Remap CAN pins to Portb alt funcs.
    nvic_setup(); //Set up some interrupts
    parm_load(); //Load stored parameters
	spi1_setup();											
    Stm32Scheduler s(TIM2); //We never exit main so it's ok to put it on stack
    scheduler = &s;
    //Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
    Stm32Can c(CAN1, CanHardware::Baud500,true);
	FunctionPointerCallback cb(CanCallback, SetCanFilters);
	CanMap cm(&c);
	CanSdo sdo(&c, &cm);
	sdo.SetNodeId(Param::GetInt(Param::NodeId));
	//store a pointer for easier access
	can = &c;
	canMap = &cm;
    canSdo = &sdo;
	c.AddCallback(&cb);
    Terminal t(USART3, termCmds);
    TerminalCommands::SetCanMap(canMap);
  
    s.AddTask(Ms10Task, 10);
    s.AddTask(Ms100Task, 100);
	s.AddTask(Ms200Task, 200);

    Param::SetInt(Param::version, 4);
    Param::Change(Param::PARAM_LAST); //Call callback one for general parameter propagation
	DigIo::POT_CS.Set();
	LoadValues();
	SetCanFilters();
	Param::SetInt(Param::MODE, MOD_OFF);

    while(1)
    {
        char c = 0;
        t.Run();
        if (sdo.GetPrintRequest() == PRINT_JSON)
        {
            TerminalCommands::PrintParamsJson(&sdo, &c);
        }
    }

    return 0;
}

