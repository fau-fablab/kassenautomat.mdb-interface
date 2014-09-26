/* *INDENT-OFF* */
// Indent funzt bei dieser Datei nicht richtig?!

/*
 *    Filename: lib/global.h
 *     Version: 0.0.2
 * Description: Globale Definition
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
#ifndef _HAVE_LIB_GLOBAL_H
#define _HAVE_LIB_GLOBAL_H
#define TRUE 1
#define true 1
#define FALSE 0
#define false 0
#ifndef cbi
#define cbi(sfr, bit)	(sfr &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit)	(sfr |= _BV(bit))
#endif
#ifdef wbi
#error wbi already defined, probably with a different syntax!
#endif
#ifndef wbi
#define wbi(sfr,bit,value)  do { if (value) { sbi(sfr,bit); } else { cbi(sfr,bit); } } while (0)
#endif

#include <util/delay.h>

void delayms (uint16_t ms)
{
	while (ms) {
		_delay_ms(1);
		ms--;
	}
}

void delayus (uint16_t us)
{
	while (us) {
		_delay_us(1);
		us--;
	}
}

inline static void delayus_inline (uint16_t us)
{
	while (us) {
		_delay_us(1);
		us--;
	}
}

#define nop() \
   asm volatile ("nop")

#endif
/* *INDENT-ON* */
