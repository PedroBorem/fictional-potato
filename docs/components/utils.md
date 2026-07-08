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
- Tipos legados de pivo.
- Enumeracoes IDP e comunicacao.
- Estruturas persistidas.

Constantes principais do bombeamento:

| Constante | Valor atual | Funcao |
| --- | --- | --- |
| `CONFIG_ACTUATION_CHANNEL_COUNT` | `4` | Numero de canais. |
| `CONFIG_ACTUATION_DEFAULT_RELAY_PULSE_MS` | `10000` | Tempo padrao para relés OFF/parada. |
| `CONFIG_ACTUATION_DEFAULT_READ_TIME_SEC` | `10` | Intervalo de leitura quando parado. |
| `CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL` | `false` | Entrada ativa em nivel baixo. |
| `CONFIG_PUMP_STAGE_1_DELAY_MS` | `10000` | Espera depois do canal 1. |
| `CONFIG_PUMP_STAGE_2_DELAY_MS` | `30000` | Espera depois do canal 2. |
| `CONFIG_PUMP_STAGE_3_DELAY_MS` | `30000` | Espera depois do canal 3. |
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
