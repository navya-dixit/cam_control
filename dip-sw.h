/*
 * Copyright (c) 2014 iWave Systems Technologies Pvt. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * @file dip-sw.h
 *
 * @brief  This file contains header implementation for DIP Switch driver.
 *
 * @ingroup Dip-switch driver 
 */

#ifndef __DIPSW_H__
#define __DIPSW_H__

unsigned char dip_sw_major_no;
static const char dip_sw_name[] = "dip_sw";
#define DS1 23
#define DS2 24
#define DS3 25
#define DS4 26

#define PIN_STATUS	22

enum {
	PIN1,
	PIN2,
	PIN3,
	PIN4,
};

#endif
