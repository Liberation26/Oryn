#include "Serial.h"

#define SERIAL_COM1 0x3F8

static inline void Out8(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char In8(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void SerialInit(void)
{
    Out8(SERIAL_COM1 + 1, 0x00);
    Out8(SERIAL_COM1 + 3, 0x80);
    Out8(SERIAL_COM1 + 0, 0x03);
    Out8(SERIAL_COM1 + 1, 0x00);
    Out8(SERIAL_COM1 + 3, 0x03);
    Out8(SERIAL_COM1 + 2, 0xC7);
    Out8(SERIAL_COM1 + 4, 0x0B);
}

void SerialWriteChar(char value)
{
    while ((In8(SERIAL_COM1 + 5) & 0x20) == 0)
    {
    }

    Out8(SERIAL_COM1, (unsigned char)value);
}

void SerialWriteString(const char* text)
{
    while (*text != 0)
    {
        if (*text == '\n')
        {
            SerialWriteChar('\r');
        }

        SerialWriteChar(*text);
        ++text;
    }
}
