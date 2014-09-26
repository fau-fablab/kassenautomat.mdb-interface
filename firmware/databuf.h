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

uint8_t hasData(databuf *b) {
	return (b->readPos < b->writePos);
}

uint8_t readData(databuf *b) {
	if (!hasData(b)) {
		FATAL("buf underrun");
	}
	return b->data[b->readPos++];
}