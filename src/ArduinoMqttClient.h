/*
  This file is part of the ArduinoMqttClient library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _ARDUINO_MQTT_CLIENT_H_
#define _ARDUINO_MQTT_CLIENT_H_

#if defined(__ZEPHYR__) && __ZEPHYR__ == 1 && defined(CONFIG_MQTT_LIB)
#include "implementations/ZephyrMqttClient.h"

using MqttClient = ZephyrMqttClient;
#else
#include "MqttClient.h"

using MqttClient = arduino::MqttClient;
#endif

#endif
