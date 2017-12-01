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


typedef enum {
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
} modem_int_state_t;

typedef enum{
	UNCONECTED,
	WAIT_CURSOR,
	DATA_SEND,
	WAIT_CONFIRM,
	CONECT_CHECK,
	CONECTED,
	CONECT_ERROR,
} modem_conect_state_t;


typedef enum{
	ZERO,
	CURSOR,
	DATA,
	SEND,
} modem_pub_state_t;

typedef struct{
	char *apn; 					
	char *user;					
	char *pass;					
} modem_config_t;

typedef struct{
	char *server_address;
	char *server_port;   
	char *client_id;     
	char *server_login;  
	char *server_pass;   
	char *topic_name;		
	char *content;			
} mqtt_config_t;



modem_conect_state_t modem_conect_state_check(void);

modem_pub_state_t modem_pub_state_check(void);

uint8_t	restarts_check(void);

void modem_conect(modem_config_t * p_modem_config, mqtt_config_t *	p_mqtt_config, const app_timer_id_t * p_timer_id);

void mqtt_publish(char *topic_name_p, char *content_p);



#endif //MQTT_H__
