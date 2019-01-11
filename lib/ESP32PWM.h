/* ESP32 PWM Library
 * Copyright (C) 2017 by Pedro Albuquerque
 *
 *
 * This file is part of a set of tools to facilitate migration form ESP8266 code to ESP32
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License v3.0
 * along with the Arduino PATinySPI Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */




#ifndef ESP32PWM_H
#define ESP32PWM_H

#include "Arduino.h"

uint8_t PWM_valueMax[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int8_t PWM_PinChannel[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

void PWM_initialize( int pin, int channel=0,uint32_t valueMax=255, int basefreq=5000 , int timer =13 ){
   PWM_PinChannel[pin] = channel;
   PWM_valueMax[pin] = valueMax;
  ledcSetup(channel, basefreq, timer);
  ledcAttachPin(pin, channel);
};

void analogWrite( uint8_t pin, uint32_t value) {
  // pulse width, 8191 from 2 ^ 13 - 1
  uint32_t width = (8191 / PWM_valueMax[pin]) * (int)_min(value, PWM_valueMax[pin]);

  // write PWM width
  ledcWrite(PWM_PinChannel[pin], width);
};

#endif
