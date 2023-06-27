//
// Created by gepsonka on 6/18/23.
//

#ifndef PICO_CONTROLLER_CRC_H
#define PICO_CONTROLLER_CRC_H

#include "stdint.h"

uint16_t crc16_ccitt(const uint8_t *data, uint16_t length);

#endif //PICO_CONTROLLER_CRC_H
