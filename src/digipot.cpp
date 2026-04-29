/*
 * This file is part of the ZombieVeter project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *               2021-2022 Damien Maguire <info@evbmw.com>
 * Yes I'm really writing software now........run.....run away.......
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

#include "digipot.h"
#include "utils.h"


void DigiPot::SetPot1Step()
	{	
		int wip1 = 0;
		wip1 = utils::change((Param::GetInt(Param::Pot1)), 0, 255, 0, 100);
		DigIo::POT_CS.Clear();
		spi_xfer(SPI1, 0);
		spi_xfer(SPI1, (Param::GetInt(Param::Pot1)));
		DigIo::POT_CS.Set();
		Param::SetInt(Param::DigiPot1, wip1);	
	}

void DigiPot::SetPot2Step()
	{	
		int wip2 = 0;
		wip2 = utils::change((Param::GetInt(Param::Pot2)), 0, 255, 0, 100);
		DigIo::POT_CS.Clear();
		spi_xfer(SPI1, 1);
		spi_xfer(SPI1, (Param::GetInt(Param::Pot2)));
		DigIo::POT_CS.Set();
		Param::SetInt(Param::DigiPot2, wip2);	
	}

void DigiPot::SetPot3Step()
	{	
		int wip3 = 0;
		wip3 = utils::change((Param::GetInt(Param::Pot3)), 0, 255, 0, 100);
		DigIo::POT_CS.Clear();
		spi_xfer(SPI1, 2);
		spi_xfer(SPI1, (Param::GetInt(Param::Pot3)));
		DigIo::POT_CS.Set();
		Param::SetInt(Param::DigiPot3, wip3);		
	}

void DigiPot::SetPot4Step()
	{	
		int wip4 = 0;
		wip4 = utils::change((Param::GetInt(Param::Pot4)), 0, 255, 0, 100);
		DigIo::POT_CS.Clear();
		spi_xfer(SPI1, 3);
		spi_xfer(SPI1, (Param::GetInt(Param::Pot4)));
		DigIo::POT_CS.Set();
		Param::SetInt(Param::DigiPot4, wip4);
	}

	