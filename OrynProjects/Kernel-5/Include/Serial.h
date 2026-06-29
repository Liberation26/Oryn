#ifndef ORYN_SERIAL_H
#define ORYN_SERIAL_H

void SerialInit(void);
void SerialWriteChar(char value);
void SerialWriteString(const char* text);

#endif
