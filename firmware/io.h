/*
 *    Filename: io.h
 *     Version: 0.0.2
 * Description: Basic IO functions
 *     License: GPLv2 or later
 *     Depends: avr-libc/io.h
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de>
 *        Date: 2007-12-12
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _HAVE_LIB_IO_IO_H
#define _HAVE_LIB_IO_IO_H
#include "./global.h"
#include <avr/io.h>

// Outputs: port, bit, inverted
// Example: #define LED(x) out(PORTB,PB1,0,x)
#define out(port, bit, inverted, state) \
	do { \
		if ((inverted) == 0) { \
			if ((state) == 0) { \
				cbi(port,bit); \
			} else {\
				sbi(port,bit); \
			}\
		} else  { \
			if ((state) == 0) { \
				sbi(port,bit); \
			} else {\
				cbi(port,bit);\
			}\
		} \
	} while(false)

#define setBit(port, bit, state) out(port, bit, 0, state)

inline uint8_t input_(uint8_t addr, uint8_t port)
{				//addr=PIN(A-E), port=P(A-E)(0-7)
	if (addr & (1 << port)) {
		return 1;
	} else {
		return 0;
	}
}

// example: #define TASTER() in(PINA,PA1,0)
inline uint8_t in(uint8_t port, uint8_t bit, uint8_t inverted)
{
	if (inverted) {
		return !input_(port,bit);
	} else {
		return input_(port,bit);
	}
}
#endif				// ifndef _HAVE_LIB_IO_IO_H
