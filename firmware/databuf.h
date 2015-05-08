/*
 *    Filename: databuf.h
 * Description: a well-tested FIFO-style data-buffer that safely exits on over- or underrun.
 *              ATTENTION: It cannot write/read more than BUFLEN bytes without resetting, because it is not realised as a ring-buffer.
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

#define BUFLEN 100
typedef struct {
	uint8_t data[BUFLEN];
	uint8_t readPos;
	uint8_t writePos;
} databuf;

void databufReset(databuf *b) {
	b->writePos=0;
	b->readPos=0;
}

void storeData(databuf *b,uint8_t d) {
	if (b->writePos >= BUFLEN) {
		FATAL("buf overrun");
	}
	b->data[b->writePos++]=d;
}

void storeHexNibbleAsAscii(databuf *b, uint8_t d) {
	storeData(b,hexNibbleToAscii((d&0xF0) >> 4));
	storeData(b,hexNibbleToAscii(d & 0x0F));
}

uint8_t hasData(databuf *b) {
	return (b->readPos < b->writePos);
}

/// read a databyte without removing it from the buffer
uint8_t peekData(databuf *b) {
	if (!hasData(b)) {
		FATAL("buf underrun");
	}
	return b->data[b->readPos];
}

/// read a databyet AND remove it from the buffer
uint8_t readData(databuf *b) {
	uint8_t data=peekData(b);
	b->readPos++;
	return data;
}

uint8_t readAsciiHexbyte(databuf *b) {
	uint8_t firstNibble=readData(b);
	if (!hasData(b)) {
		FATAL("missing second nibble");
	}
	return asciiToHex(firstNibble,readData(b));
}