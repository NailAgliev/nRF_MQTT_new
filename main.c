/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/** @file
 * @defgroup uart_example_main main.c
 * @{
 * @ingroup uart_example
 * @brief UART Example Application main file.
 *
 * This file contains the source code for a sample application using UART.
 *
 */
 
 
 

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_uart.h"

#include "nrf_drv_clock.h"

#include "app_timer.h"


#include "SEGGER_RTT.h"

#include "app_scheduler.h"

//#define ENABLE_LOOPBACK_TEST  /**< if defined, then this example will be a loopback test, which means that TX should be connected to RX to get data loopback. */

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

#define SCHED_MAX_EVENT_DATA_SIZE 16
#define SCHED_QUEUE_SIZE 64

#define RX_PIN 17
#define TX_PIN 18
#define	UART_PIN_DISCONNECTED   0xFFFFFFFF

APP_TIMER_DEF(uart_timer);

uint8_t modem_data[128];

uint8_t index = 0;

void (*p_func)(void *p_event_data, uint16_t event_size);

char *apn 					= "internet.mts.ru";
char *user					= "mts";
char *pass					= "mts";
char *server_address= "m20.cloudmqtt.com";
char *server_port   = "14974";

//const nrf_drv_timer_t TIMER = NRF_DRV_TIMER_INSTANCE(0);



enum {
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






static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}



bool modem_s_q_check()
{
	uint8_t comma 		=	0;
	while(modem_data[comma] != 0x2C)
	{
		comma++;
	}
	if(comma > 7)
	{
		return true;
	}
	return false;
}

bool modem_reg_chck()
{
	if(modem_data[9] == '1')
	{
		return true;
	}
	return false;
}

bool cgatt_check()
{
	if(modem_data[8] == '1')
	{
		return true;
	}
	return false;
		
}

void at_write(char second[])         //отправка комад модулю
{
		const char *first  = "AT";
    const char *third	 = "\r\n";
	
    printf("%s%s%s", first, second, third);
	
		SEGGER_RTT_printf(0, "%s%s%s", first, second, third);
		
}

void at_write_apn(const char apn[],const char user[],const char pass[])         //отправка комад модулю
{
		const char *start  = "AT+CSTT=";
    const char *end	 = "\r\n";
		
		
	
    printf("%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);
	
		SEGGER_RTT_printf(0, "%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);
		
}

void at_write_tcp(const char server_address[],const char server_port[])         //отправка комад модулю
{
		const char *start  = "AT+CIPSTART=\"TCP\",\"";
    const char *end	 = "\r\n";
		
		
	
    printf("%s%s\",\"%s\"%s", start, server_address, server_port, end);
	
		SEGGER_RTT_printf(0, "%s%s\",\"%s\"%s", start, server_address, server_port, end);
		
}




void modem_init()
{
	
	switch(modem_int_state)
	{
		case AT:  									 //Проверка доступен ли модуль
			{
				//at_write("");
				app_uart_flush();
				at_write("");
				app_timer_start(uart_timer, APP_TIMER_TICKS(1000), NULL);
				break;
			}
		case CFUN:
			{
				app_uart_flush();
				at_write("+CFUN=0");
				break;
			}
		case CFUN_1: 								 //Рестарт модуля
			{
				app_uart_flush();
				at_write("+CFUN=1,1");
				break;
			}
		case ATE:										//No echo mode
			{
				app_uart_flush();
				at_write("E0");
				break;
			}
		case ATV:										//Числовой формат ответов
			{
				app_uart_flush();
				at_write("V0");
				break;
			}
		case CMEE:  							 	//Кодовый формат ошибок
			{
				app_uart_flush();
				at_write("+CMEE=1");
				break;
			}
		case CPIN_CHECK:						
			{
				app_uart_flush();
				at_write("+CPIN=?");
				break;
			}
		case CSQ_CHECK: 						//Проверка силы сигнала
			{
				app_uart_flush();
				at_write("+CSQ");
				break;
			}
		case CREG_CHECK:						//Проверка рекгестрации в сети
			{
				app_uart_flush();
				at_write("+CREG?");
				break;
			}
		case CIPSHUT:								//TCP restart
			{
				app_uart_flush();
				at_write("+CIPSHUT");
				break;
			}
		case CGATT_CHECK:            //проверка готовности модуля для установления связи
			{
				app_uart_flush();
				at_write("+CGATT?");
				break;
			}
		case CIPRXGET:							//Ручное получение полученных сообщений
			{
				app_uart_flush();
				at_write("+CIPRXGET=1");
				break;
			}
		case CIPQSEND:							//Режим отправки без потверждения получения		
			{
				app_uart_flush();
				at_write("+CIPQSEND=1");
				break;
			}
		case CSTT:									//Конфигурация APN
			{
				app_uart_flush();
				at_write_apn(apn, user, pass);
				break;
			}
		case CIICR:									//Влючение GPRS
			{
				app_uart_flush();
				at_write("+CIICR");
				break;
			}
		case CIFSR:
			{
				app_uart_flush();
				at_write("+CIFSR");
				break;
			}
		case CIPSTART:
			{
				app_uart_flush();
				at_write_tcp(server_address, server_port);
				break;
			}
		default:
			break;
	}
}


void serial_scheduled_ex (void * p_event_data, uint16_t event_size)      //работает по прирыванию
{
	if(modem_int_state < OK)
	{
		switch(modem_int_state)
	{
		case AT:  																																					//Проверка доступен ли модуль
			{
//				size_t size = sizeof(modem_data);
				//nrf_mtx_unlock(&p_event_data->p_ctx->read_lock);
				//nrf_serial_read(p_event_data, &modem_data, sizeof(modem_data), &size, 0);
				//SEGGER_RTT_printf(0, "%s", modem_data);
				if(modem_data[0] == ('O')|| modem_data[0] == ('0')|| modem_data[0] == ('A'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CFUN;					
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					app_timer_start(uart_timer, APP_TIMER_TICKS(1000), NULL);
					break;
				}
			}
		case CFUN:
			{
				if(modem_data[0] == ('O')|| modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CFUN_1;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CFUN_1: 								 																													//Рестарт модуля
			{
				if(modem_data[0] == ('O')|| modem_data[0] == ('+')|| modem_data[0] == ('0'))
				{
															
					modem_int_state = CPIN_CHECK;			
					memset(modem_data, 0, sizeof(modem_data));
					app_timer_start(uart_timer, APP_TIMER_TICKS(11000), NULL);
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CPIN_CHECK:																																		//Проверка пин кода
			{
				if(modem_data[0] == '0')
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = ATE;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_init();
					break;
				}
			}
		case ATE:																																							//No echo mode
			{
				if(modem_data[0] == ('O')|| modem_data[0] == ('0') || modem_data[0] == ('A'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = ATV;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case ATV:																																							//Числовой формат ответов
			{
				if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CMEE;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CMEE:  																																			 	//Кодовый формат ошибок
			{
				if(modem_data[0] == '0')
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CSQ_CHECK;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CSQ_CHECK: 																																	//Проверка силы сигнала
			{
				if(modem_data[0] == '+')
				{
					bool s = modem_s_q_check();
					if(s == true)
					{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CREG_CHECK;
					modem_init();
					break;
					}
					else
					{
						printf("Low signal ERROR");
						modem_int_state = ERROR;
						break;
					}
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CREG_CHECK:																																	//Проверка рекгестрации в сети
			{
				if(modem_data[0] == '+')
				{
					bool s = modem_reg_chck();
					if(s == true)
					{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIPSHUT;
					modem_init();
					break;
					}
					else
					{
						app_timer_start(uart_timer, APP_TIMER_TICKS(1000), NULL);
						break;
					}
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIPSHUT:																																			//TCP restart
			{
				if(modem_data[0] == 'S')
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CGATT_CHECK;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CGATT_CHECK:            //проверка готовности модуля для установления связи
			{
				if(modem_data[0] == '+')
				{
					bool s;
					s = cgatt_check();
					if(s == true)
					{
						memset(modem_data, 0, sizeof(modem_data));
						modem_int_state = CGATT_CHECK_OK;
						break;
					}
					else
						memset(modem_data, 0, sizeof(modem_data));
						app_timer_start(uart_timer, APP_TIMER_TICKS(1000), NULL);
						break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
				
			}
		case CGATT_CHECK_OK:
			{
				if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIPRXGET;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIPRXGET:							//Ручное получение полученных сообщений
			{
				if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIPQSEND;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIPQSEND:							//Режим отправки без потверждения получения		
			{
				if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CSTT;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CSTT:									//Конфигурация APN
			{
				if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIICR;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIICR:									//Влючение GPRS
			{
					if(modem_data[0] == ('0'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIFSR;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIFSR:									//Влючение GPRS
			{
					if(strlen(modem_data) > 6)
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = CIPSTART;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		case CIPSTART:									//Подключение к серверу
			{
					if(modem_data[0] == ('C'))
				{
					memset(modem_data, 0, sizeof(modem_data));
					modem_int_state = OK;
					modem_init();
					break;
				}
				else
				{
					memset(modem_data, 0, sizeof(modem_data));
					break;
				}
			}
		
		default:
			break;			
	}
	}
}




void rx_read()
{
//	uint32_t err_code;
//	uint8_t buf[128];
//	uint8_t i = 0;
//	while(err_code == NRF_SUCCESS)
//	{
//		app_uart_get(buf+i);
//		i++;
//	}
//	
	
	app_uart_get(modem_data+index);
	index++;
	if(modem_data[index-1] == 0x0A)
	{
		index = 0;
		SEGGER_RTT_printf(0, "%s", modem_data);
		app_sched_event_put(NULL, NULL, serial_scheduled_ex);
	}
	
}




void uart_event_handle(app_uart_evt_t * p_event)
{
	switch(p_event->evt_type)
	{
		case APP_UART_DATA_READY:
		{
			if(modem_int_state == AT)
			{
				app_timer_stop_all();
				app_sched_event_put(NULL, NULL, serial_scheduled_ex);				
			}
			rx_read();
		}
		default:
			break;
	}
}


void timer_timeout_handler(void * p_context)
{
	app_sched_event_put(NULL, NULL, modem_init);
}

static void lfclk_config(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    uint32_t err_code;
		
		scheduler_init();
	
		lfclk_config();
	
   // bsp_board_leds_init();
		
//		nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
//    err_code = nrf_drv_timer_init(&TIMER, &timer_cfg, timer_event_handler);
//		if(err_code == NRF_SUCCESS)
//		SEGGER_RTT_printf(0, "TIMER\n\r");
//    APP_ERROR_CHECK(err_code);
//		
		err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
//		
		err_code = app_timer_create(&uart_timer, APP_TIMER_MODE_SINGLE_SHOT, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);

    const app_uart_comm_params_t comm_params =
      {
					RX_PIN,
          TX_PIN,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          NRF_UART_BAUDRATE_115200
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_event_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

    APP_ERROR_CHECK(err_code);

    //printf("\r\nStart: \r\n");
		SEGGER_RTT_printf(0, "init\r\n");
			
		//printf("Hello World \r\n");
			
		modem_init();
			
			
			
		while (true)
    {
			app_sched_execute();
    }

//    while (true)
//    {
//        uint8_t cr;
//        while (app_uart_get(&cr) != NRF_SUCCESS);
//        while (app_uart_put(cr) != NRF_SUCCESS);

//        if (cr == 'q' || cr == 'Q')
//        {
//            printf(" \r\nExit!\r\n");

//            while (true)
//            {
//                // Do nothing.
//            }
//        }
//    }

}


/** @} */
