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

int IGN = 0;
int CHARGE = 0;
int16_t Counter = 0;
int CAN_ON = 0;
int Mot_Temp = 0;

static void Ms10Task(void)
{
    //Set timestamp of error message
    ErrorMessage::SetTime(rtc_get_counter_val());
	Param::SetInt(Param::MODE, (Param::GetInt(Param::Mode)));
	if (Param::GetInt(Param::Mode)) Counter = 0;
	Param::SetInt(Param::RunTime, (Counter/5));
}


//sample 100ms task
static void Ms100Task(void)
{
    int Pot_Val = 0;
	DigIo::led_out.Toggle();
    iwdg_reset();
    float cpuLoad = scheduler->GetCpuLoad();
    Param::SetFloat(Param::CPU_LOAD, cpuLoad / 10);
	IGN = DigIo::IGN.Get();
	CHARGE = DigIo::CHARGE.Get();
	Param::SetInt(Param::IGN, IGN);
	Param::SetInt(Param::CHARGE, CHARGE);
	Pot_Val = utils::change(Mot_Temp, 20, 100, 255, 128);
	if (Pot_Val > 255) Pot_Val = 255;
	if (Pot_Val < 128) Pot_Val = 128;
	Param::SetInt(Param::Pot1, Pot_Val);
	
	if (((Param::GetInt(Param::IGN))) || ((Param::GetInt(Param::CHARGE))) || (Param::GetInt(Param::TimeOut) == 0) || (CAN_ON)) 
	{
		Param::SetInt(Param::Mode, 1);
		DigIo::BMS1.Set();
		DigIo::BMS2.Set();
		Param::SetInt(Param::BMS1, 1);
		Param::SetInt(Param::BMS2, 1);
	}
	else 
	{
		Param::SetInt(Param::Mode, 0);
		if (((Param::GetInt(Param::RunTime))/60) >= (Param::GetInt(Param::TimeOut)))
		{
			DigIo::BMS1.Clear();
			DigIo::BMS2.Clear();
			Param::SetInt(Param::BMS1, 0);
			Param::SetInt(Param::BMS2, 0);
		}
	}
	canMap->SendAll();
	Can_Tasks();
	LoadValues();
	tim3_setup();
	tim4_setup();
}

static void Ms200Task(void)
{
	DigiPot::SetPot1Step();
	DigiPot::SetPot2Step();
	DigiPot::SetPot3Step();
	DigiPot::SetPot4Step();
	Counter++;
	CAN_ON = 0;
}



void Can_Tasks()
{
	
	
	uint8_t bytes[8];
    bytes[0]= (Param::GetInt(Param::Mode));
    bytes[1]= ((Counter/5) >> 8);
    bytes[2]= Counter/5;
	bytes[3]= (Param::GetInt(Param::Pot1));
	bytes[4]= (Param::GetInt(Param::Pot2));
	bytes[5]= (Param::GetInt(Param::Pot3));
	bytes[6]= (Param::GetInt(Param::Pot4));
	bytes[7]= 0x00;
    
    can->Send(0x722, bytes, 8); //Send on CAN1	
	
}
	
void DecodeCAN(int id, uint32_t* data)
{
	uint8_t* bytes = (uint8_t*)data;
	switch (id)
	{
		case 0x1AE:
			CAN_ON = bytes[0];
			break;
		case 0x501:
			//Param::SetInt(Param::Pot1, bytes[0]);
			Mot_Temp = bytes[0];
			break;
		case 0x502:
			Param::SetInt(Param::Pot2, bytes[0]);
			break;
		case 0x503:
			Param::SetInt(Param::Pot3, bytes[0]);
			break;
		case 0x504:
			Param::SetInt(Param::Pot4, bytes[0]);
			break;
		case 0x303:
			Param::SetInt(Param::PWM3_CH3, bytes[0]);
			Param::SetInt(Param::Tim3_Frequency, bytes[1]);
			Param::SetInt(Param::Tim3_3_DC, bytes[2]);
			break;
		case 0x401:
			Param::SetInt(Param::PWM4_CH1, bytes[0]);
			Param::SetInt(Param::Tim4_Frequency, bytes[1]);
			Param::SetInt(Param::Tim4_1_DC, bytes[2]);
			break;	
		default:
			break;
	}
			
}



static void SetCanFilters()
{
	can->RegisterUserMessage(0x605); //Can SDO
	can->RegisterUserMessage(0x1AE); //OI Control Message
	can->RegisterUserMessage(0x501); //POT1 Control Message
	can->RegisterUserMessage(0x502); //POT2 Control Message
	can->RegisterUserMessage(0x503); //POT3 Control Message
	can->RegisterUserMessage(0x504); //POT4 Control Message
	can->RegisterUserMessage(0x303); //PWM3_Ch3 Control Message
	can->RegisterUserMessage(0x401); //PWM4_Ch1 Control Message
	
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
		case Param::Tim3_Frequency:
		case Param::PWM3_CH3:
		case Param::Tim3_3_DC:
		case Param::Tim4_Frequency:
		case Param::PWM4_CH1:
		case Param::Tim4_1_DC:
			LoadValues();
			tim3_setup();
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

