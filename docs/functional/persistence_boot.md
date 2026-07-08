# Persistencia e Boot

## Sequencia de Inicializacao

O firmware atual inicia em `app_main()` e chama `system_manager_init()`.

Ordem atual:

1. Inicializa RTC DS3231.
2. Inicializa NVS via `data_app_init()`.
3. Carrega `act_config` da NVS.
4. Se nao existir configuracao, cria valores padrao.
5. Migra o nivel ativo de status para o padrao atual, nivel baixo.
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
| `DATA_TYPE_COMM_MAIN_MODE` | `comm_main_mode` | Canal principal para eventos espontaneos: `RF` ou `MQTT`. |

## Configuracao Padrao

| Campo | Valor atual |
| --- | --- |
| `relay_pulse_time_ms` | `10000` |
| `read_time_sec` | `10` |
| `status_active_level` | `false`, equivalente a nivel baixo |
| `comm_main_mode` | `RF` |

## Dados Legados

O componente `data_app` ainda cria e preserva chaves antigas de pivo, como:

- `action`
- `pivot_config`
- `rush_config`
- `sector_config`
- `gps_config`
- `history`
- `comm_main_mode`

Essas chaves nao participam da regra atual de bombeamento. Elas foram mantidas para evitar migracao destrutiva e para servir como base caso algum recurso seja reaproveitado na segunda etapa.

## Comportamento Seguro no Boot

- Todos os relés sao desenergizados na inicializacao do `gpio_actuator`.
- A bomba nao parte automaticamente no boot.
- A partida depende de comando explicito.
- Se uma configuracao antiga tiver entrada ativa em nivel alto, o `system_manager` corrige para nivel baixo e salva novamente.
