#ifndef __simple_obd_test_H
#define __simple_obd_test_H

#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <FreematicsPlus.h>
#include <FreematicsGPS.h>
#include <httpd.h>
#include "config.h"
#include "telelogger.h"
#include "telemesh.h"
#include "teleclient.h"
#ifdef BOARD_HAS_PSRAM
#include "esp_himem.h"
#endif
#if ENABLE_OLED
#include "FreematicsOLED.h"
#endif

// states
#define STATE_STORAGE_READY 0x1
#define STATE_OBD_READY 0x2
#define STATE_GPS_READY 0x4
#define STATE_MEMS_READY 0x8
#define STATE_NET_READY 0x10
#define STATE_NET_CONNECTED 0x20
#define STATE_WORKING 0x40
#define STATE_STANDBY 0x100

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/list.h>
#include <freertos/queue.h>
#include <freertos/portmacro.h>
#include <freertos/semphr.h>

#define USE_SERIAL 0
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define STACK_SIZE	1024 // in words (= 4*STACK_SIZE bytes)
#define buff_SIZE 500
#define time_size 100
#define USE_OBD 0
#define NBR_GPS_POINTS 10


#endif /* __simple_obd_test_H */
