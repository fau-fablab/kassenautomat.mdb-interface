/*
 *    Filename: main.c
 * Description: bus interface between PC and MDB-bus-device
 *     License: GPLv2 or later
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de>
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
#include <avr/wdt.h>
#include <avr/pgmspace.h>
void fatal_real(const char * PROGMEM str, uint8_t d);
#define FATAL(x) fatal_real(PSTR(x),0)
#define FATAL_h(x,d) fatal_real(PSTR(x),d)


#include "./io.h"
#include "./uart.h"

#define LED_DDR DDRB				// Hier DDR der Test-LED einstellen!
#define LED_PORT PORTB				// Hier PORT der Test-LED einstellen!
#define LED_PIN PB7				// Hier PIN der Test-LED einstellen!
#define LED(x) out(LED_PORT,LED_PIN,0,x)	// DO NOT CHANGE


uint8_t hexNibbleToAscii(uint8_t x);
void fatal_real(const char * PROGMEM str, uint8_t d) {
	uart0_tx_pstr(PSTR("\nFATAL: "));
	uart0_tx_pstr(str);
	uart0_tx_blocking(hexNibbleToAscii(d>>4));
	uart0_tx_blocking(hexNibbleToAscii(d&0xF));
	uart0_tx_blocking('\n');
	while (1) {}
}

#include "databuf.h"

databuf cmd, resp;
enum {READ_CMD, SEND_CMD, READ_RESP, SEND_RESP} state;
uint8_t cmdBytesSent=0;
uint8_t respChk=0;
uint8_t respLen=0;
uint8_t timeout=0;

uint8_t asciiNibbleToHex(uint8_t ascii) {
	if (('0' <= ascii) && (ascii <= '9')) {
		return ascii-'0';
	} else if (('A' <= ascii) && (ascii <= 'F')) {
		return ascii-'A'+10;
	} else {
		FATAL_h("invalid hexstring",ascii);
		return 0;
	}
}

uint8_t asciiToHex(uint8_t high, uint8_t low) {
	return (asciiNibbleToHex(high) << 4) | asciiNibbleToHex(low);
}

uint8_t hexNibbleToAscii(uint8_t x) {
	if (x<10) {
		return '0'+x;
	} else if (x<16) {
		return 'A' + x - 10;
	} else {
		FATAL("hex too large");
		return 0;
	}
}

int main(void) {
	wdt_enable(WDTO_120MS);
	
	LED_DDR |= (1<<LED_PIN); 		// setting LED-PIN to output
//	OSCCAL = 0b01101001;			// kalibriere auf 9,6MHz
	uart0_init();//(38400,1,1);
	uart1_init();
	DDRD |= (1<<PD3);
	DDRE |= (1<<PE1);
	databufReset(&cmd);
	databufReset(&resp);
	// erstmal ein byte senden, damit txReady funktioniert
	uart0_tx('\n');
	uart1_tx(0xFF,1);
	uart0_tx_pstr(PSTR("INFO:booting\n"));
	while(1) {				// Testcode: LED-Blinker mit 10Hz
		wdt_reset();
		LED_PORT &= ~(1<<LED_PIN);
		switch (state) {
			case READ_CMD: // get command from PC
				LED_PORT |= (1<<LED_PIN);
				if (uart0_rx_ready()) {
					uint8_t d=uart0_rx();
					//uart0_tx_blocking(d);
					if (d=='\r' || d=='\n') {
						state=SEND_CMD;
						cmdBytesSent=0;
					} else {
						storeData(&cmd,d);
					}
				}
				break;
			case SEND_CMD: // send command to device
				if (uart0_rx_ready() || uart1_rx_ready()) {
					FATAL("rx while SEND CMD");
				}
				if (uart1_tx_ready()) {
					if (hasData(&cmd)) {
						uint8_t firstNibble=readData(&cmd);
						if (!hasData(&cmd)) {
							FATAL("missing second nibble");
						}
						// send databyte. set bit8 if it is the first byte
						uart1_tx(asciiToHex(firstNibble,readData(&cmd)), (cmdBytesSent==0));
						cmdBytesSent++;
					} else {
						databufReset(&cmd);
						// response R:<hexdata>
						storeData(&resp,'R');
						storeData(&resp,':');
						state=READ_RESP;
						respChk=0;
						respLen=0;
						timeout=0;
					}
				}
				break;
			case READ_RESP: // receive response from device, send ACK
				if (uart0_rx_ready()) {
					FATAL("rx0 while READ RESP");
				}
				if (uart1_rx_ready()) {
					timeout=0;
					uint16_t d=uart1_rx();
					respLen++;
					storeData(&resp,hexNibbleToAscii((d&0xF0) >> 4));
					storeData(&resp,hexNibbleToAscii(d & 0x0F));
					
					if (d&(1<<8)) { // bit8 is set - this is the end of the response
						uint8_t okay=0;
						uint8_t receivedNAK=0;
						state=SEND_RESP;
						if (respLen==1) {
							if (d==0x100) {
								okay=1; // ACK
							} else if (d==0x1FF) {
								// NAK empfangen - nicht mit NAK antworten!
								receivedNAK=1;
							} else {
								// Unsinn empfangen
							}
							okay = (d == 0x100); // ACK empfangen?
						} else {
							// answer contains data - validate checksum
							okay = ((d & 0xff) == respChk);
						}
						
						if (okay) {
							if (respLen>1) {
								// ACK nur wenn die Gegenstelle mehr zu sagen hatte als "ACK"
								uart1_tx(0x00,0); // ACK
							}
						} else {
							databufReset(&resp);
							storeData(&resp,'R');
							if (receivedNAK) {
								// NAK empfangen - nicht mit NAK antworten!
								storeData(&resp,'N');
							} else {
								storeData(&resp,'E');
								uart1_tx(0xFF,0);
							}
						}
					}
					
					respChk += (d&0xFF);

				} else {
					// uart1_rx not ready
					_delay_us(100);
					timeout++;
					if (timeout >= 50) {
						databufReset(&resp);
						storeData(&resp,'R');
						storeData(&resp,'T');
						state=SEND_RESP;
					}
				}
				break;
			case SEND_RESP: // send response to PC
				if (uart0_rx_ready() || uart1_rx_ready()) {
					FATAL("rx while sending");
				}
				if (uart0_tx_ready()) {
					if (hasData(&resp)) {
						uart0_tx(readData(&resp));
					} else {
						uart0_tx('\n');
						databufReset(&resp);
						state=READ_CMD;
					}
				}
				break;
			default:
				FATAL("unknown state");
		}
	}
	return 0;
}
