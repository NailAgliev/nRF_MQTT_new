#ifndef MQTT_H__
#define MQTT_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_scheduler.h"
#include "SEGGER_RTT.h"

#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "nrf_uart.h"

#include "nrf_drv_clock.h"
#include "app_timer.h"


#define SCHED_MAX_EVENT_DATA_SIZE 16
#define SCHED_QUEUE_SIZE 64

#define RX_PIN 17
#define TX_PIN 18
#define	UART_PIN_DISCONNECTED   0xFFFFFFFF

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256

APP_TIMER_DEF(uart_timer);

static enum {
	AT,
	CFUN,
	CFUN_1,
	ATE,
	ATV,
	CMEE,
	CPIN_CHECK,
	CSQ_CHECK,
	CREG_CHECK,
	CIPSHUT,
	CGATT_CHECK,
	CGATT_CHECK_OK,
	CIPQSEND,
	CIPRXGET,
	CSTT,
	CIICR,
	CIFSR,
	CIPSTART,
	OK, //15
	ERROR,
} modem_int_state;

static enum{
	REDY,
	WAIT_CURSOR,
	WAIT_CONFIRM,
} modem_send_state;

void modem_conect();

#endif //MQTT_H__
