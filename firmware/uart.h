/*
 *    Filename: lib/io/serial/uart.h
 *     Version: 0.0.1
 * Description: Beispieldatei
 *     License: GPLv2 or later
 *     Depends: io.h
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de> and additional Features by Philipp Hörauf
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
#ifndef _HAVE_LIB_IO_SERIAL_UART_H
#define _HAVE_LIB_IO_SERIAL_UART_H

#ifndef PE0
#define PE0 UPE0
#endif

#ifndef PE1
#define PE1 UPE1
#endif

#include "./io.h"
#include <avr/pgmspace.h>

void uart0_init(void);
void uart0_tx_blocking(unsigned char byte);
uint8_t uart0_tx_ready(void);
void uart0_tx(uint8_t byte);
uint8_t uart0_rx(void);
void uart0_tx_str(unsigned char* string);
uint8_t uart0_rx_ready(void);
void uart1_init(void);
uint8_t uart1_tx_ready(void);
void uart1_tx(uint8_t byte, uint8_t bit8);
uint16_t uart1_rx(void);
void uart1_tx_str(unsigned char* string);
uint8_t uart1_rx_ready(void);


// UBRR=(F_CPU)/(baud*16L), aber gerundet:
// UBRR=(obere Formel*2 +1)/2

void uart0_init() {
	uint16_t _ubrr=25;//(((F_CPU*2)/(baud*16L)) +1)/2;
	uint8_t rx=1;
	uint8_t tx=1;
	// baud
	UBRR0H = _ubrr>>8;
	UBRR0L = _ubrr;
	
	// RX/TX aktivieren
	UCSR0B = (rx << RXEN0)|(tx << TXEN0);
	
	// 8 Bit Daten; 1 Stopbit; Synchronous Operation
	UCSR0C = (0 << USBS1) | (1 << UCSZ01) | (3 << UCSZ00);
}

void uart0_tx_blocking(unsigned char byte) {
	while (!(UCSR0A & (1 << UDRE0))) {} // noch etwas blockiered
	UDR0=byte;
}

uint8_t uart0_tx_ready(void) {
	return (UCSR0A & (1 << UDRE0));
}
void uart0_tx(uint8_t byte) {
	if (!uart0_tx_ready()) {
		FATAL("!uart0_tx_ready");
	}
	UDR0=byte;
}

uint8_t uart0_rx(void) {
//	while (!(UCSRA & (1 << RXC))) {} es wird nicht gewartet! kein delay erlaubt!
	return UDR0;
}

void uart0_tx_str(unsigned char* string) {
	uint16_t i=0;
	while (string[i] != 0x00) {
		uart0_tx_blocking(string[i]);	
		i++;
	}
}

void uart0_tx_pstr(const char* PROGMEM str) {
	uint8_t byte;
	while ((byte=pgm_read_byte(str++))) {
		uart0_tx_blocking(byte);
	}
}


uint8_t uart0_rx_ready(void) {
	if (UCSR0A & (1<<RXC0)) {
		if (UCSR0A & (1<<PE0)) {
// 			Parity Error stillschweigend verwerfen
// 			volatile uint8_t tmp=UDR;
// 			tmp++;
			return 0;
		} else {
			return 1;
		}
	} else {
		return 0;
	}
}


void uart1_init(void) {
	uint16_t _ubrr=103;
	// baud
	UBRR1H = _ubrr>>8;
	UBRR1L = _ubrr;
	
	// RX/TX aktivieren
	UCSR1B = (1 << RXEN1)|(1 << TXEN1) | (1 << UCSZ12);
	
	// 9 Bit Daten; 2 Stopbits; Synchronous Operation
	UCSR1C = (1<<USBS1) |  (1 << UCSZ11) | (3 << UCSZ10);
}

uint8_t uart1_tx_ready(void) {
	return (UCSR1A & (1 << UDRE1));
}
void uart1_tx(uint8_t byte, uint8_t bit8) {
	if (!uart1_tx_ready()) {
		FATAL("!uart1_tx_ready");
	}
	UCSR1B &= ~(1<<TXB81);
	if (bit8) {
		UCSR1B |= (1<<TXB81);
	}
	UDR1=byte;
}

uint16_t uart1_rx(void) {
//	while (!(UCSRA & (1 << RXC))) {} es wird nicht gewartet! kein delay erlaubt!
	//volatile uint8_t status=UCSR1A;
	volatile uint8_t bit8=!!(UCSR1B&(1<<RXB81));
	return UDR1 | (bit8<<8);
}

// void uart1_tx_str(unsigned char* string) {
// 	uint16_t i=0;
// 	while (string[i] != 0x00) {
// 		uart1_tx_blocking(string[i]);	
// 		i++;
// 	}
// }

uint8_t uart1_rx_ready(void) {
	if (UCSR1A & (1<<RXC1)) {
		if (UCSR1A & (1<<PE1)) {
// 			Parity Error stillschweigend verwerfen
// 			volatile uint8_t tmp=UDR;
// 			tmp++;
			return 0;
		} else {
			return 1;
		}
	} else {
		return 0;
	}
}

#endif
