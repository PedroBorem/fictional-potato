/*
 * eco_mode.c
 *
 *  Created on: 15 de out. de 2023
 *      Author: soil-dev
 */
#include "eco_mode.h"
#include "rtc_app.h"
#include "FreeRTOS_defines.h"
#include "log.h"

#include <string.h>

#define ECO_MODE_TAG	"eco_mode"

static TaskHandle_t xTask_eco_mode = NULL;
static eco_mode_config eco_mode = {};
static app_callback eco_mode_callback = NULL;

static void eco_mode_task(void* arg)
{
	bool alredy_off = false;

	time_t current_time = 0;
	//pivot_actions current_action = {};

	while(1)
	{
		current_time = rtc_app_get_timestamp(false);

		if(eco_mode.start_time < eco_mode.end_time)
		{
			if(current_time >= eco_mode.start_time
			&& current_time <= eco_mode.end_time)
			{
				if(alredy_off == false)
				{
					if(eco_mode_callback != NULL)
					{
						eco_mode_callback("#01-eco_mode-002-000-eco_mode", COMM_MQTT);
					}

					alredy_off = true;
				}
			}
			else if(alredy_off == true)
			{
				// todo : voltar as configs
			}
		}
		else
		{
			if(current_time >= eco_mode.end_time
			&& current_time <= eco_mode.start_time)
			{
				if(alredy_off == false)
				{
					if(eco_mode_callback != NULL)
					{
						//todo
						eco_mode_callback("#01-eco_mode-002-000-eco_mode", COMM_MQTT);
					}
					alredy_off = true;
				}
			}
			else if(alredy_off == true)
			{
				// todo : voltar as configs
				alredy_off = false;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(15000)); // 15 seconds
	}
}

void eco_mode_start(eco_mode_config current_eco_mode)
{
	if(current_eco_mode.start_time == 0
	|| current_eco_mode.end_time == 0)
	{
		eco_mode_stop();
	}
	else
	{
		memcpy(&eco_mode, &current_eco_mode, sizeof(eco_mode));
		xTaskCreate(&eco_mode_task,
					ECO_MODE_TASK_NAME,
					ECO_MODE_TASK_SIZE,
					NULL,
					ECO_MODE_TASK_PRIORITY,
					&xTask_eco_mode);
	}
}

void eco_mode_stop(void)
{
	if(xTask_eco_mode != NULL)
	{
		vTaskDelete(xTask_eco_mode);
		xTask_eco_mode = NULL;
	}
}

void eco_mode_register_callback(const app_callback callback)
{
	if(callback != NULL)
	{
		eco_mode_callback = callback;
	}
}

/*
 * #include <stdio.h>
#include <time.h>

int main() {
    int timestamp = 1697421033;
    int horas, minutos, segundos, totalSegundos;

    // Definindo o deslocamento horário de Brasília em segundos (GMT-3)
    int deslocamento_horario = -3 * 3600;

    timestamp += deslocamento_horario; // Ajuste para o horário de Brasília

    // Calcular as horas, minutos e segundos
    horas = (timestamp / 3600) % 24; // Obter o número de horas e aplicar o módulo 24 para obter o valor dentro de um dia.
    minutos = (timestamp % 3600) / 60;
    segundos = timestamp % 60;

    // Calcular o total de segundos
    totalSegundos = (horas * 3600) + (minutos * 60) + segundos;

    printf("Timestamp: %d segundos\n", timestamp);
    printf("Hora em Brasília: %02d:%02d:%02d\n", horas, minutos, segundos);
    printf("Total de segundos: %d segundos\n", totalSegundos);

    return 0;
}
Neste código, adicionamos a variável totalSegundos para armazenar a soma dos segundos contidos nas horas, minutos e segundos. Além disso, calculamos e exibimos o valor de totalSegundos no final.

 *
 * */

