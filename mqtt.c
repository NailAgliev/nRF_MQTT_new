#include "mqtt.h"

static  uint8_t 						 modem_data[128]; 		//Строка полученая от модема
static  uint8_t 						 index = 0; 					//индекс для строки
static	modem_config_t			 modem_config;    		//локальная структура modem_config 
static	mqtt_config_t 			 mqtt_config;					//локальная структура mqtt_config	
static	modem_int_state_t		 modem_int_state;			//состояние инициализации
static	modem_conect_state_t modem_conect_state;  //состояние подключения
static	modem_pub_state_t 	 modem_pub_state;			//состояние отправки

static 	uint8_t						 	 timer_flag;
static 	uint8_t	restarts = 0;
app_timer_id_t timer_id;

static void send_string(char *string_p)																																			 //отправка строки
{
	if(timer_flag == 0)
		{
			app_timer_start(timer_id, APP_TIMER_TICKS(5000), NULL);
			timer_flag = 1;
		}
	for(uint8_t i = 0; i < strlen(string_p); i++)
		{
			app_uart_put((uint8_t)*(string_p + i));
		}
}


static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


static bool modem_s_q_check()//проверка силы сигнала
{					
	uint8_t comma 		=	0;					 //индекс члена масива содержащего запятую
	while(modem_data[comma] != 0x2C) //ищем запятую в массиве		
	{					
		comma++;					
	}		
	//если сила меньше 10(один знак) то запятая будет на седьмой позиции если больше то запятая сместиться на восьмую позицию
  //получаем +CSQ: 18,0
	if(comma > 7)	
	{					
		return true;					
	}					
	return false;					
}					
					
static bool modem_reg_chck()//проверка регистрации в сети
{					
																
	if(modem_data[9] == '1')	//получаем +CREG: 0,1		еденица значит что модем в сети зарегестрирован
	{					
		return true;					
	}					
	return false;					
}					
					
static bool cgatt_check()//проверка доступности GPRS
{					
	if(modem_data[8] == '1')					
	{					
		return true;					
	}					
	return false;					
							
}					

modem_conect_state_t modem_conect_state_check()//проверка состояния подключения
{
	return modem_conect_state;
}

modem_pub_state_t modem_pub_state_check()//проверка состояния отправки
{
	return modem_pub_state;
}
uint8_t	restarts_check()
{
	return restarts;
}
					
static void at_write(char second[])//отправка команд модулю
{					
		const char *first  = "AT";					
    const char *third	 = "\r\n";					
					char string[120];
	
    sprintf(string, "%s%s%s", first, second, third);					
						
		SEGGER_RTT_printf(0, "%s%s%s", first, second, third);					
		
		send_string(string);
}					
					
static void at_write_apn(const char apn[],const char user[],const char pass[])//подключение к провайдеру
{					
		const char *start  = "AT+CSTT=";					
    const char *end	 = "\r\n";					
					char string[120];		
							
						
    sprintf(string, "%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);					
						
		SEGGER_RTT_printf(0, "%s\"%s\",\"%s\",\"%s\"%s", start, apn, user, pass, end);					
		
		send_string(string);
}					
					
static void at_write_tcp(const char server_address[],const char server_port[])//подключение к серверу
{					
		const char *start  = "AT+CIPSTART=\"TCP\",\"";					
    const char *end	 = "\r\n";
					char string[120];
							
							
						
    sprintf(string, "%s%s\",\"%s\"%s", start, server_address, server_port, end);					
						
		SEGGER_RTT_printf(0, "%s%s\",\"%s\"%s", start, server_address, server_port, end);					
		
		send_string(string);
}					
					
void mqtt_connect(char *client_id, char *server_login, char *server_pass)//авторизация на сревере 
{									                                                                                           
	const uint8_t con_flag					= 0x10;									                                                   
	//все члены масивы стандарстные последние два бита отвечают за длительност подключения в секундах сейчас стоит	1 hour									 
	const uint8_t con_fix_heder[10] = {0x00, 0x04, 'M', 'Q', 'T', 'T', 0x04, 0xC2, 0x8C, 0xA0};								 	
										                                                                                         
	uint16_t client_id_length 			= strlen(client_id);									                                     
													                                                                                   
	uint16_t server_login_length 		= strlen(server_login);									                                   
													                                                                                   
	uint16_t server_pass_length  		= strlen(server_pass);									                                   
	//стандартные служебные байты не меняются всегда 16
	uint8_t package_length      		= (client_id_length + server_login_length + server_pass_length + 16	); 
										                                                                                         
	const char *start  = "AT+CIPSEND=";		//ат команда отправки	через сим на сервер						                                                       						                                                                                         
  const char *end	 	 = "\r\n";		
	char string[120];
										                                                                                         
	union{									                                                                                   
		uint16_t length;									                                                                       
		struct{									                                                                                 
					uint8_t b_lsb;									                                                                   
					uint8_t b_msb;									                                                                   
		}byte;									                                                                                 
	}bidl;									                                                                                   
										                                                                                         
	bidl.length = client_id_length;									                                                           
										                                                                                         
	union{									                                                                                   
		uint16_t length;									                                                                       
		struct{									                                                                                 
					uint8_t b_lsb;									                                                                   
					uint8_t b_msb;									                                                                   
		}byte;									                                                                                 
	}blogl;									                                                                                   
										                                                                                         
	blogl.length = server_login_length;									                                                       
										                                                                                         
	union{									                                                                                   
		uint16_t length;									                                                                       
		struct{									                                                                                 
					uint8_t b_lsb;									                                                                   
					uint8_t b_msb;									                                                                   
		}byte;									                                                                                 
	}bpassl;									                                                                                 
										                                                                                         
	bpassl.length = server_pass_length;									                                                       
											                                                                                       
	switch (modem_conect_state)									                                                               
		{									                                                                                       
			case UNCONECTED:									                                                                           
				{									                                                                                   
					app_uart_flush();									                                                                 
					sprintf(string, "%s%d%s", start, package_length+2, end);		
					send_string(string);
					modem_conect_state = WAIT_CURSOR;									                                                 
					break;									                                                                           
				}									                                                                                   
			case DATA_SEND:									                                                                       
				{										                                                                                 
					app_uart_flush();									                                                                 
														                                                                                 
					app_uart_put(con_flag);									                                                           
														                                                                                 
					app_uart_put(package_length);									                                                     
														                                                                                 
					for(uint8_t i = 0; i < sizeof(con_fix_heder); i++)									                               
					{									                                                                                 
						app_uart_put(con_fix_heder[i]);									                                                 
					}									                                                                                 
														                                                                                 
					app_uart_put(bidl.byte.b_msb);									                                                   
					app_uart_put(bidl.byte.b_lsb);									                                                   
														                                                                                 
					send_string(client_id);									                                                         
														                                                                                 
					app_uart_put(blogl.byte.b_msb);									                                                   
					app_uart_put(blogl.byte.b_lsb);									                                                   
														                                                                                 
					send_string(server_login);									                                                       
														                                                                                 
					app_uart_put(bpassl.byte.b_msb);									                                                 
					app_uart_put(bpassl.byte.b_lsb);									                                                 
														                                                                                 
					send_string(server_pass);									                                                       
														                                                                                 
					break;									                                                                           
				}									                                                                                   
			case WAIT_CONFIRM:									                                                                   
				{									                                                                                   
					app_uart_flush();									                                                                 
					at_write("+CIPRXGET=2,4");	//отпровляем для полученя 4 байт данных								                                                       
					break;									                                                                           
				}									                                                                                   
			case CONECTED:									                                                                       
			{									                                                                                     
				break;									                                                                             
			}									                                                                                     
		}									                                                                                       
}									                                                                                           
									                                                                                           
void mqtt_publish(char *topic_name_p, char *content_p)//отправка сообщения на сервер
{
	if(modem_conect_state == CONECTED)
		{
	
	mqtt_config.topic_name 				= topic_name_p;		
	mqtt_config.content 					= content_p;		
			
	const uint8_t pub_flag				= 0x31;	//с ретейном 31 без ретэйна 30						
	uint16_t topic_name_length 		= strlen(topic_name_p);								
	uint16_t content_length 			= strlen(content_p);								
	uint8_t package_length      	= (topic_name_length + content_length +2);	
			
	union{					
		uint16_t length;					
		struct{					
					uint8_t b_lsb;					
					uint8_t b_msb;					
		}byte;					
	}btopicl;			
						
			
	const char *start  						= "AT+CIPSEND=";							
  const char *end	 							= "\r\n";						
	char string[120];	
	switch (modem_pub_state)					
		{					
		case ZERO:																																															 //начало отправки
				{					
					app_uart_flush();
					timer_flag = 0;
					sprintf(string, "%s%d%s", start, package_length+2, end);	
					send_string(string);
					modem_pub_state = CURSOR;					
					break;					
				}					
		case DATA:																																															 //после получения корсора отправка пакета 
				{					
					app_uart_flush();
					
					btopicl.length = topic_name_length;					
					
					app_uart_put(pub_flag);	
										
					app_uart_put(package_length);	
										
					app_uart_put(btopicl.byte.b_msb);					
					app_uart_put(btopicl.byte.b_lsb);					
										
					send_string(topic_name_p);					

					send_string(content_p);		
						
					SEGGER_RTT_printf(0, "%s", content_p);

					modem_pub_state = SEND;
					break;
				}					
		}
	}
}
					
					
static void modem_publish()//проверка подключения отправка подтверждения подключения
{					
	if(modem_int_state == OK)					
	{					
		if(modem_conect_state == UNCONECTED)																																			
		{					
			mqtt_connect(mqtt_config.client_id, mqtt_config.server_login, mqtt_config.server_pass);					
		}					
		if(modem_conect_state == CONECTED)																																				
		{					
			mqtt_publish(mqtt_config.topic_name, mqtt_config.content);					
		}					
	}					
}					
					
					
static void modem_init()//Инициализация и подключение к серверу
{					
						
	switch(modem_int_state)					
	{					
		case AT:  									 																																						 //Проверка доступен ли модуль					
			{																		                                                                   
				app_uart_flush();														                                                         
				at_write("");		
				timer_flag = 1;
				app_timer_start(timer_id, APP_TIMER_TICKS(1000), NULL);														                 
				break;														                                                                   
			}														                                                                           
		case CFUN:														                                                                   
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CFUN=0");														                                                     
				break;														                                                                   
			}														                                                                           
		case CFUN_1: 								 																																						 //Рестарт модуля					
			{																                                                                       
				app_uart_flush();														                                                         
				at_write("+CFUN=1,1");														                                                   
				break;														                                                                   
			}		
		case CPIN_CHECK:																																											   //Проверка PIN
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CPIN=?");														                                                     
				break;														                                                                   
			}				
		case ATE:																																																 //No echo mode					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("E0");														                                                           
				break;														                                                                   
			}														                                                                           
		case ATV:																														 																		 //Числовой формат ответов					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("V0");														                                                           
				break;														                                                                   
			}														                                                                           
		case CMEE:  																																														 //Кодовый формат ошибок					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CMEE=1");														                                                     
				break;														                                                                   
			}														                                                                           														                                                                         
		case CSQ_CHECK: 																																												 //Проверка силы сигнала					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CSQ");														                                                         
				break;														                                                                   
			}														                                                                           
		case CREG_CHECK:																																												 //Проверка рекгестрации в сети					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CREG?");														                                                       
				break;														                                                                   
			}														                                                                           
		case CIPSHUT:																																														 //TCP restart					
			{																																		
				app_uart_flush();																																		
				at_write("+CIPSHUT");																																		
				break;																																		
			}																																		
		case CGATT_CHECK:         																																						   //проверка готовности модуля для установления связи					
			{																																		
				app_uart_flush();																																		
				at_write("+CGATT?");																																		
				break;																																		
			}																																		
		case CIPRXGET:																																													 //Ручное получение полученных сообщений					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CIPRXGET=1");														                                                 
				break;														                                                                   
			}														                                                                           
		case CIPQSEND:																																													 //Режим отправки без потверждения получения							
			{														                                                                           
				app_uart_flush();														                                                         
				at_write("+CIPQSEND=1");														                                                 
				break;														                                                                   
			}														                                                                           
		case CSTT:																																															 //Конфигурация APN					
			{														                                                                           
				app_uart_flush();														                                                         
				at_write_apn(modem_config.apn, modem_config.user, modem_config.pass);														     
				break;														                                                                   
			}														                                                                           
		case CIICR:																																															 //Влючение GPRS					
			{					                                                                                             
				app_uart_flush();					                                                                           
				at_write("+CIICR");					                                                                         
				break;					                                                                                     
			}					                                                                                             
		case CIFSR:																																															 //Получение присвоеного IP
			{					
				app_uart_flush();					
				at_write("+CIFSR");					
				break;					
			}					
		case CIPSTART:																																													 //Подключение к серверу
			{					
				app_uart_flush();					
				at_write_tcp(mqtt_config.server_address, mqtt_config.server_port);					
				break;					
			}					
		default:					
			break;					
	}					
}					
					
					
static void serial_scheduled_ex (void * p_event_data, uint16_t event_size)//работает по прерыванию
{					                                                                                                   
	if(modem_int_state < OK)					                                                                         
	{					                                                                                                 
		switch(modem_int_state)					                                                                         
	{					                                                                                                 
		case AT:	//Проверка доступен ли модуль
			{														                                                                           
				if(modem_data[0] == ('0'))							
				{	
					app_timer_stop_all();
					timer_flag = 0;
					memset(modem_data, 0, sizeof(modem_data));	//чистим модем дата													                             
					modem_int_state = CFUN;																			                                       
					modem_init();														                                                           
					break;														                                                                 
				}														                                                                         
				else														                                                                     
				{														                                                                         
					memset(modem_data, 0, sizeof(modem_data));														                             
					app_timer_start(timer_id, APP_TIMER_TICKS(1000), NULL);														               
					break;														                                                                 
				}														                                                                         
			}														                                                                           
		case CFUN:
			{														                                                                           
				if(modem_data[0] == ('O')|| modem_data[0] == ('0'))														                       
				{	
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CFUN_1: 		//Рестарт модуля
			{														                                                                           
				if(modem_data[0] == ('O')|| modem_data[0] == ('+')|| modem_data[0] == ('0'))												 		
				{														                                                                         
					app_timer_stop_all();
					timer_flag = 0;					
					modem_int_state = CPIN_CHECK;																	                                     
					memset(modem_data, 0, sizeof(modem_data));														                             
					app_timer_start(timer_id, APP_TIMER_TICKS(11000), NULL);														             
					break;														                                                                 
				}														                                                                         
				else														                                                                     
				{														                                                                         
					memset(modem_data, 0, sizeof(modem_data));														                             
					break;														                                                                 
				}														                                                                         
			}														                                                                           
		case CPIN_CHECK:		//Проверка пин кода
			{														                                                                           
				if(modem_data[0] == '0')														                                                 
				{		
					app_timer_stop_all();
					timer_flag = 0;					
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
		case ATE:		//No echo mode
			{														                                                                           
				if(modem_data[0] == ('O')|| modem_data[0] == ('0') || modem_data[0] == ('A')									)			 		
				{		
					app_timer_stop_all();
					timer_flag = 0;					
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
		case ATV:	//Числовой формат ответов
			{														                                                                           
				if(modem_data[0] == ('0'))														                                               
				{	
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CMEE:  //Кодовый формат ошибок
			{														                                                                           
				if(modem_data[0] == '0')														                                                 
				{	
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CSQ_CHECK: 	//Проверка силы сигнала
			{														                                                                           
				if(modem_data[0] == '+')														                                                 
				{														                                                                         
					bool s = modem_s_q_check();														                                             
					if(s == true)														                                                           
					{	
					app_timer_stop_all();
					timer_flag = 0;						
					memset(modem_data, 0, sizeof(modem_data));														                             
					modem_int_state = CREG_CHECK;														                                           
					modem_init();														                                                           
					break;														                                                                 
					}														                                                                       
					else														                                                                   
					{														                                                                       
						SEGGER_RTT_printf(0, "Low signal ERROR");
						app_timer_stop_all();
						timer_flag = 0;						
						memset(modem_data, 0, sizeof(modem_data));
						app_timer_start(timer_id, APP_TIMER_TICKS(1000), NULL);														                                           
						//modem_int_state = ERROR;														                                             
						break;														                                                               
					}														                                                                       
				}														                                                                         
				else														                                                                     
				{														                                                                         
					memset(modem_data, 0, sizeof(modem_data));														                             
					break;														                                                                 
				}														                                                                         
			}														                                                                           
		case CREG_CHECK:	//Проверка рекгестрации в сети
			{														                                                                           
				if(modem_data[0] == '+')														                                                 
				{														                                                                         
					bool s = modem_reg_chck();														                                             
					if(s == true)														                                                           
					{	
					app_timer_stop_all();
					timer_flag = 0;						
					memset(modem_data, 0, sizeof(modem_data));														                             
					modem_int_state = CIPSHUT;														                                             
					modem_init();														                                                           
					break;														                                                                 
					}														                                                                       
					else														                                                                   
					{														                                                                       
						app_timer_start(timer_id, APP_TIMER_TICKS(1000), NULL);														             
						break;														                                                               
					}														                                                                       
				}														                                                                         
				else														                                                                     
				{														                                                                         
					memset(modem_data, 0, sizeof(modem_data));														                             
					break;														                                                                 
				}														                                                                         
			}														                                                                           
		case CIPSHUT:		//TCP restart
			{														                                                                           
				if(modem_data[0] == 'S')														                                                 
				{		
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CGATT_CHECK:   //проверка готовности модуля для установления связи
			{														                                                                           
				if(modem_data[0] == '+')														                                                 
				{														                                                                         
					bool s;														                                                                 
					s = cgatt_check();														                                                     
					if(s == true)														                                                           
					{		
						app_timer_stop_all();
						timer_flag = 0;						
						memset(modem_data, 0, sizeof(modem_data));														                           
						modem_int_state = CGATT_CHECK_OK;														                                     
						break;														                                                               
					}														                                                                       
					else														                                                                   
						memset(modem_data, 0, sizeof(modem_data));														                           
						app_timer_start(timer_id, APP_TIMER_TICKS(1000), NULL);														             
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
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CIPRXGET:		//Ручное получение полученных сообщений
			{														                                                                           
				if(modem_data[0] == ('0'))														                                               
				{				
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CIPQSEND:	//Режим отправки без потверждения получения		
			{																	                                                                     
				if(modem_data[0] == ('0'))																	                                         
				{		
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CSTT:		//Конфигурация APN
			{														                                                                           
				if(modem_data[0] == ('0'))														                                               
				{			
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CIICR:		//Влючение GPRS
			{														                                                                           
					if(modem_data[0] == ('0'))														                                             
				{					
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CIFSR:	//Получение IP адреса (мы его никуда не принимаем и не записываемся нужен для подключения)
			{														                                                                           
					if(strlen(modem_data) > 6)														                                             
				{				
					app_timer_stop_all();
					timer_flag = 0;					
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
		case CIPSTART:	//Подключение к серверу
			{					                                                                                             
					if(modem_data[0] == ('C'))					                                                               
				{			
					app_timer_stop_all();
					timer_flag = 0;					
					memset(modem_data, 0, sizeof(modem_data));					                                               
					modem_int_state = OK;					                                                                     
					modem_init();					                                                                             
					modem_publish();					                                                                         
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
					                                                                                                   
					                                                                                                   
					                                                                                                   
					                                                                                                   
static void rx_read()//собирает в строку
{						                                                                                                 
	app_uart_get(modem_data+index);					                                                                   
	index++;					                                                                                         
	if(modem_data[index-1] == 0x0A)					                                                                   
	{					                                                                                                 
		index = 0;					                                                                                     
		SEGGER_RTT_printf(0, "%s", modem_data);					                                                         
		app_sched_event_put(NULL, NULL, serial_scheduled_ex);					                                           
	}					                                                                                                 
}					                                                                                                   
					                                                                                                   
					                                                                                                   
static void rx_red_confirm()//проверка успешности авторизации на сервере
{
	app_uart_get(modem_data+index);
	index++;
	if(index == 27)
	{
		index = 0;
		if(modem_data[23] == 0x00)
			{
				modem_conect_state = CONECTED;
				memset(modem_data, 0, sizeof(modem_data));
				modem_publish();
			}
			else if(modem_data[23] == 0x01 || modem_data[23] == 0x02 || modem_data[23] == 0x03 || modem_data[23] == 0x04 || modem_data[23] == 0x05)  //когда будет время добавить ошибки подключения
			{
				modem_conect_state = CONECT_ERROR;
			}
	}
}


static void serial_scheduled_conect (void * p_event_data, uint16_t event_size)//хендлер прерыванию юарт во время подключения
{
	switch (modem_conect_state)
		{
		case WAIT_CURSOR://ждем курсор
				{
					app_uart_get(modem_data);
					if(modem_data[0] == '>')
						{
							memset(modem_data, 0, sizeof(modem_data));
							modem_conect_state = DATA_SEND;
							mqtt_connect(mqtt_config.client_id, mqtt_config.server_login, mqtt_config.server_pass);
						}
						else if(modem_data[0] == '+')
						{
							modem_conect_state = CONECT_ERROR;
							SEGGER_RTT_printf(0, "SEND ERROR :DISCONECTED\r\n");
							break;
						}
					break;
				}
			case DATA_SEND://ждем DATA ACCEPT
				{
					app_uart_get(modem_data);
					if(modem_data[0] == 'D')
						{
							memset(modem_data, 0, sizeof(modem_data));
							modem_conect_state = WAIT_CONFIRM;
						}
						break;
				}
			case WAIT_CONFIRM://ждем ответа от сервера
				{
					app_uart_get(modem_data);
					if(modem_data[0] == '+')
						{
							memset(modem_data, 0, sizeof(modem_data));
							mqtt_connect(mqtt_config.client_id, mqtt_config.server_login, mqtt_config.server_pass);
						}
						else
						{
							memset(modem_data, 0, sizeof(modem_data));
						}
						break;
				}
			case CONECT_CHECK://обробатываем ответ
				{
					rx_red_confirm();
					break;
				}
						
		}
}

static void serial_scheduled_publish (void * p_event_data, uint16_t event_size)//хендлер прерыванию юарт во время отправки
{
	switch (modem_pub_state)
		{
		case CURSOR:	//ждем курсор
			{
				app_uart_get(modem_data);
				if(modem_data[0] == '>')
					{
						app_timer_stop_all();
						timer_flag = 0;					
						memset(modem_data, 0, sizeof(modem_data));
						modem_pub_state = DATA;
						mqtt_publish(mqtt_config.topic_name, mqtt_config.content);
						break;
					}
					else if(modem_data[0] == '+')
					{
						app_timer_stop_all();
						timer_flag = 0;					
						memset(modem_data, 0, sizeof(modem_data));
						modem_pub_state = ZERO;
						modem_conect_state = UNCONECTED;
						modem_int_state = AT;
						modem_init();
						restarts++;
						SEGGER_RTT_printf(0, "SEND ERROR :DISCONECTED\r\n");
						break;
					}
					break;

			}
		case SEND:	//ждем подтверждения отправки
			{
				app_uart_get(modem_data);
				if(modem_data[0] == 'D')
					{
						memset(modem_data, 0, sizeof(modem_data));
						nrf_delay_ms(5000);
						modem_pub_state = ZERO;
						break;
					}
			}
		}
}

static void uart_event_handle(app_uart_evt_t * p_event)
{
	switch(p_event->evt_type)
	{
		case APP_UART_DATA_READY:
		{
			if(modem_pub_state == CURSOR  || modem_pub_state == SEND)	//при отправке
			{
				app_timer_stop_all();
				timer_flag = 0;
				app_sched_event_put(NULL, NULL, serial_scheduled_publish);
				break;
			}
			if(modem_conect_state != UNCONECTED  && modem_conect_state != CONECTED)	//при подключении
			{
				app_timer_stop_all();
				timer_flag = 0;
				app_sched_event_put(NULL, NULL, serial_scheduled_conect);
				break;
			}
			if(modem_int_state == AT)	//во время ожидания ответа от модема
			{
				app_sched_event_put(NULL, NULL, serial_scheduled_ex);
			}
			rx_read();
			break;
		}
		case APP_UART_TX_EMPTY:
		{
			if(modem_conect_state == WAIT_CONFIRM)	//ждем ответ от сервера
					{
						modem_conect_state = CONECT_CHECK;
						break;
					}
				}
		
		default:
			break;
	}
}


static void timer_timeout_handler(void * p_context)
{
	timer_flag = 0;
	app_sched_event_put(NULL, NULL, modem_init);
	if(modem_pub_state == CURSOR  || modem_pub_state == SEND)	//при отправке
			{
				app_timer_stop_all();
				timer_flag = 0;
				app_sched_event_put(NULL, NULL, serial_scheduled_publish);
			}
}

static void lfclk_config(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}


void modem_conect(modem_config_t * p_modem_config, mqtt_config_t *	p_mqtt_config, const app_timer_id_t * p_timer_id)
{
		
		timer_id = *p_timer_id;
	
		uint32_t err_code;
	
		modem_config.apn 					 = p_modem_config->apn;
		modem_config.user					 = p_modem_config->user;
		modem_config.pass					 = p_modem_config->pass;
		                           
		mqtt_config.server_address = p_mqtt_config->server_address;
	  mqtt_config.server_port    = p_mqtt_config->server_port;   
	  mqtt_config.client_id      = p_mqtt_config->client_id;     
	  mqtt_config.server_login   = p_mqtt_config->server_login;  
	  mqtt_config.server_pass    = p_mqtt_config->server_pass;   
		mqtt_config.topic_name		 = p_mqtt_config->topic_name;		
	 	mqtt_config.content				 = p_mqtt_config->content;				
	
	
		scheduler_init();
	
		lfclk_config();
	
		err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
		
		err_code = app_timer_create(&timer_id, APP_TIMER_MODE_SINGLE_SHOT, timer_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
		const app_uart_comm_params_t comm_params =
      {
					RX_PIN,
          TX_PIN,
          NULL,
          NULL,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          NRF_UART_BAUDRATE_9600
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

