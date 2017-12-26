//
// Created by armadillo2 on 26/12/17.
//

#ifndef RIC_INTERFACE_CRC8_H
#define RIC_INTERFACE_CRC8_H

#include <stdio.h>
#include <iostream>

typedef uint8_t byte;

class Crc8
{

private:
    const unsigned char CRC7_POLY_ = 0x91;
    unsigned char crc_table_[256];


    unsigned char get_byte_crc(unsigned char val)
    {
        unsigned char j;

        for (j = 0; j < 8; j++)
        {
            if (val & 1)
                val ^= CRC7_POLY_;
            val >>= 1;
        }
        return val;
    }

    /* build crc table */
    void init()
    {
        int i;
        // fill an array with CRC values of all 256 possible bytes
        for (i = 0; i < 256; i++)
            crc_table_[i] = get_byte_crc(i);
    }

public:

    unsigned char get_crc(byte message[], size_t size)
    {
        init();
        unsigned char i, crc = 0;
        for (i = 0; i < size; i++)
            crc = crc_table_[crc ^ message[i]];
        return crc;
    }
};

#endif //RIC_INTERFACE_CRC8_H
