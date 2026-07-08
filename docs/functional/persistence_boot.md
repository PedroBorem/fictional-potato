# Persistencia e Boot

## Sequencia de Inicializacao

O firmware atual inicia em `app_main()` e chama `system_manager_init()`.

Ordem atual:

1. Inicializa RTC DS3231.
2. Inicializa NVS via `data_app_init()`.
3. Carrega `act_config` da NVS.
4. Se nao existir configuracao, cria valores padrao.
5. Migra `act_config` quando o tamanho persistido nao corresponde ao formato atual.
6. Aplica configuracao de atuacao.
7. Carrega modo principal de comunicacao; RF e o padrao.
8. Inicializa GPIO e tarefa de atuacao.
9. Inicializa RF UART, GPRS UART e parser IDP.
10. Le e registra o status inicial dos 4 canais.

## Dados Novos do Produto

| Data type | Chave NVS | Funcao |
| --- | --- | --- |
| `DATA_TYPE_ACTUATION_ACTIONS` | `act_actions` | Ultimo comando aceito pela camada de atuacao. |
| `DATA_TYPE_ACTUATION_CONFIG` | `act_config` | Configuracao da camada de atuacao. |
| `DATA_TYPE_REASON_HANG_UP` | `reason_hangup` | Ultimo pacote `#28` de desligamento do sistema. |
| `DATA_TYPE_COMM_MAIN_MODE` | `comm_main_mode` | Canal principal para eventos espontaneos: `RF` ou `MQTT`. |

## Configuracao Padrao

| Campo | Valor atual |
| --- | --- |
| `relay_pulse_time_ms` | `10000` |
| `idle_read_time_sec` | `10` |
| `status_active_level` | `false`, equivalente a nivel baixo |
| `ramp_1_delay_sec` | `5` |
| `stage_1_delay_sec` | `10` |
| `ramp_2_delay_sec` | `5` |
| `stage_2_delay_sec` | `30` |
| `ramp_3_delay_sec` | `5` |
| `stage_3_delay_sec` | `30` |
| `ramp_4_delay_sec` | `5` |
| `stage_4_delay_sec` | `0` |
| `status_publish_time_min` | `1` |
| `comm_main_mode` | `RF` |

Se a NVS tiver uma versao antiga de `act_config`, o firmware detecta a diferenca de tamanho, aplica a configuracao padrao completa e salva novamente.

Rampas persistidas ou recebidas com valor `0` sao normalizadas para `5 s`. Isso impede validacao instantanea da entrada de status antes da estabilizacao do motor/optoacoplador.

## Dados Reservados

Permanecem criados/preservados para adaptacao futura:

- `net_config`: IDPs 2 e 6 e comunicacao GPRS/MQTT real.
- `rush_config` e `rush_state`: codigo congelado, fora do build.
- `reboot_config`: futuro reaproveitamento do IDP 24.
- `s_date`, `s_off_date` e `s_start_state`: futuro agendamento por horario.
- `history`: futuro IDP 12.

## Limpeza da NVS Antiga

No boot, `data_app` remove de forma idempotente:

- `action` e `pivot_config`;
- `sector_config` e `gps_config`;
- `s_angle` e `s_off_angle`;
- `timestamp`;
- `status_barrier`, `virtual_barrier` e `phy_barrier`;
- `initial_angle` e `manual_counter`.

Essas chaves nao sao mais criadas, carregadas nem salvas pelo firmware.

## Comportamento Seguro no Boot

- Todos os relés sao desenergizados na inicializacao do `gpio_actuator`.
- A bomba nao parte automaticamente no boot.
- A partida depende de comando explicito.
- O valor padrao das entradas e nivel baixo, mas o IDP 3 permite configurar nivel baixo ou alto.
- O firmware publica o ultimo `#28` em todo boot.
- Se `esp_reset_reason()` indicar brownout, o firmware salva e publica `#28` com `REASON=brownout`.
- Se o boot acontecer com a ultima acao persistida ainda em ON, o firmware salva e publica `#28` com `PHASE=was_commanded_on` e o `RESET_REASON` real.
