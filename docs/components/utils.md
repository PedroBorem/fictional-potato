# Componente `utils`

## Situacao

Ativo no build atual.

## Funcao

Agrupa definicoes compartilhadas do projeto.

## `project_config.h`

Contem:

- Versao de firmware.
- Constantes do novo produto.
- Tipos de atuacao.
- Enumeracoes IDP e comunicacao.
- Estruturas persistidas.

Foram removidas as estruturas compartilhadas de configuracao de pivo, setor, GPS, barreiras e agendamento por angulo. Alguns tipos antigos continuam temporariamente declarados apenas porque `rush_mode.c`, `scheduling.c`, `system_monitoring.c` e os formatos reservados de historico/agendamento por data ainda aguardam decisao.

Constantes principais do bombeamento:

| Constante | Valor atual | Funcao |
| --- | --- | --- |
| `CONFIG_ACTUATION_CHANNEL_COUNT` | `4` | Numero de canais. |
| `CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS` | `10000` | Tempo padrao para relés OFF/parada. |
| `CONFIG_ACTUATION_DEFAULT_IDLE_READ_TIME_SEC` | `10` | Cadencia interna de leitura quando parado. |
| `CONFIG_ACTUATION_DEFAULT_STATUS_PUBLISH_MIN` | `1` | Intervalo de envio periodico do `#00$`, em minutos. |
| `CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL` | `false` | Entrada ativa em nivel baixo. |
| `CONFIG_PUMP_RAMP_1_DELAY_MS` | `5000` | Rampa padrao do motor 1. |
| `CONFIG_PUMP_STAGE_1_DELAY_MS` | `10000` | Espera depois do canal 1. |
| `CONFIG_PUMP_RAMP_2_DELAY_MS` | `5000` | Rampa padrao do motor 2. |
| `CONFIG_PUMP_STAGE_2_DELAY_MS` | `30000` | Espera depois do canal 2. |
| `CONFIG_PUMP_RAMP_3_DELAY_MS` | `5000` | Rampa padrao do motor 3. |
| `CONFIG_PUMP_STAGE_3_DELAY_MS` | `30000` | Espera depois do canal 3. |
| `CONFIG_PUMP_RAMP_4_DELAY_MS` | `5000` | Rampa padrao do motor 4. |
| `CONFIG_PUMP_STAGE_4_DELAY_MS` | `0` | Espera depois do canal 4. |
| `CONFIG_PUMP_MONITOR_INTERVAL_MS` | `500` | Intervalo de monitoramento em operacao. |
| `CONFIG_PUMP_STAGE_LOG_INTERVAL_MS` | `10000` | Intervalo de log detalhado do timer de partida. |
| `CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS` | `1000` | Intervalo do heartbeat `......` durante partida. |
| `CONFIG_PUMP_STOP_RELAY_TIME_MS` | `10000` | Tempo dos relés OFF na parada. |

## `FreeRTOS_defines.h`

Define nomes, pilhas e prioridades de tasks.

No firmware atual, a task relevante nova e:

- `ACTUATION_APP_TASK_NAME`

## `log.h`

Define macros de log.

Observacao: `LOG_DBG_ERROR` ainda tenta enviar erro por `gprs_uart_send_event()` como simbolo fraco. O fluxo principal novo responde erros pelo callback do `system_manager` usando pacote `#99-...$`.
