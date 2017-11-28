#include "mqtt.h"



static  uint8_t modem_data[128];

static  uint8_t index = 0;

static  char *apn 					= "internet.mts.ru";
static  char *user					= "mts";
static  char *pass					= "mts";
static  char *server_address= "m20.cloudmqtt.com";
static  char *server_port   = "14974";

static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

static bool modem_s_q_check()
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

static bool modem_reg_chck()
{
	if(modem_data[9] == '1')
	{
		return true;
	}
	return false;
}

static bool cgatt_check()
{
	if(modem_data[8] == '1')
	{
		return true;
	}
	return false;
		
}

static void at_write(char second[])         //отправка комад модулю
{
		const char *first  = "AT";
    const char *third	 = "\r\n";
	
    printf("%s%s%s", first, second, third);
	
		SEGGER_RTT_printf(0, "%s%s%s", first, second, third);
		
}

static void at_write_apn(const char apn[],const char user[],const char pass[])         //отправка комад модулю
{
		const char *start  = "AT+CSTT=";
    const char *end	 = "\r\n";
		
		
	
    printf("%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);
	
		SEGGER_RTT_printf(0, "%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);
		
}

static void at_write_tcp(const char server_address[],const char server_port[])         //отправка комад модулю
{
		const char *start  = "AT+CIPSTART=\"TCP\",\"";
    const char *end	 = "\r\n";
		
		
	
    printf("%s%s\",\"%s\"%s", start, server_address, server_port, end);
	
		SEGGER_RTT_printf(0, "%s%s\",\"%s\"%s", start, server_address, server_port, end);
		
}

static void modem_init()
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


static void serial_scheduled_ex (void * p_event_data, uint16_t event_size)      //работает по прирыванию
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




static void rx_read()
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

static void uart_event_handle(app_uart_evt_t * p_event)
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


static void timer_timeout_handler(void * p_context)
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

void modem_conect()
{
	 uint32_t err_code;
		
		scheduler_init();
	
		lfclk_config();
	
		err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
		
		err_code = app_timer_create(&uart_timer, APP_TIMER_MODE_SINGLE_SHOT, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
		const app_uart_comm_params_t comm_params =
      {
					RX_PIN,
          TX_PIN,
          NULL,
          NULL,
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
			
		modem_init();
	
}

