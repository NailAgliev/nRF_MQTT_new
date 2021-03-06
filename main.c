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
#include "nrf.h"
#include "bsp.h"
#include "nrf_uart.h"

#include "mqtt.h"

#include "nrf_delay.h"
#include "nrf_temp.h"

APP_TIMER_DEF(modem_timer);

modem_config_t my_modem_config = {    //стуктура для подключения к сети
	.apn 					 	= "internet.mts.ru",
	.user					 	= "mts",
	.pass					 	= "mts",
};

mqtt_config_t my_mqtt_config = {    //структура подключения к серверу MQTT
	.server_address	= "m20.cloudmqtt.com",
	.server_port   	= "14974",
	.client_id     	= "modem",
	.server_login  	= "iviqnyll",
	.server_pass   	= "TOOXoaHFQ8vi",
	.topic_name		 	= "init",
	.content			 	= "OK",
};


char *topic = "temp"; //название топика температуры
char content[120];		 	//строка для отправки

modem_conect_state_t  modem_conect_state; //состояние подключения
modem_pub_state_t 		modem_pub_state;		//состояние отправки

/**
 * @brief Function for main application entry.
 */
int main(void)
{
	
		int32_t volatile temp;
	
		int16_t volatile index = 0;
	
    nrf_temp_init();
		
		uint8_t	restarts = 0;
	
		modem_conect(&my_modem_config, &my_mqtt_config, &modem_timer);  //инициализация модема и подключение к серверу

    while (true)
    {
				app_sched_execute();		//выполнение отложеных ивентов

        NRF_TEMP->TASKS_START = 1; /** Start the temperature measurement. */

        /* Busy wait while temperature measurement is not finished, you can skip waiting if you enable interrupt for DATARDY event and read the result in the interrupt. */
        /*lint -e{845} // A zero has been given as right argument to operator '|'" */
        while (NRF_TEMP->EVENTS_DATARDY == 0)
        {
					app_sched_execute();
        }
        NRF_TEMP->EVENTS_DATARDY = 0;

        /**@note Workaround for PAN_028 rev2.0A anomaly 29 - TEMP: Stop task clears the TEMP register. */
        temp = (nrf_temp_read() / 4);

        /**@note Workaround for PAN_028 rev2.0A anomaly 30 - TEMP: Temp module analog front end does not power down when DATARDY event occurs. */
        NRF_TEMP->TASKS_STOP = 1; /** Stop the temperature measurement. */	
				
							

				modem_conect_state =  modem_conect_state_check(); //получение состояния подключения
				modem_pub_state 	 =	modem_pub_state_check();		//получение состояния отправки
				restarts           = 	restarts_check();
				if(modem_conect_state == CONECTED && modem_pub_state == ZERO)
				{

					sprintf(content, "Actual temperature: %d Index: %d Was restarted %d times\r\n", (int)temp, (int)index, (int)restarts);
		
					mqtt_publish(topic, content);
					
					index++;
					if(index >= 10000)
					{
						index = 0;
					}
				}

    }
	
}

/** @} */
