# Componente `gpio_actuator`

## Situacao

Ativo no build atual.

## Funcao

Driver de GPIO para os 4 pares de relés ON/OFF e as 4 entradas de status.

## Pinagem

| Canal | ON | OFF | Status |
| --- | --- | --- | --- |
| 1 | `GPIO_NUM_13` | `GPIO_NUM_12` | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_11` | `GPIO_NUM_10` | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_9` | `GPIO_NUM_8` | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_16` | `GPIO_NUM_35` | `GPIO_NUM_4` |

## Niveis Eletricos

Relés:

- `GPIO_ACT_RELAY_ENABLE = 0`
- `GPIO_ACT_RELAY_DISABLE = 1`

Entradas:

- `status_active_level = false` interpreta nivel baixo como ON.

## APIs Principais

| API | Funcao |
| --- | --- |
| `gpio_actuator_init()` | Configura GPIOs de saida e entrada. |
| `gpio_actuator_config()` | Aplica configuracao de leitura/atuacao. |
| `gpio_actuator_set()` | Aplica comandos por canal; mantido como API geral de atuacao. |
| `gpio_actuator_enable_on_relay()` | Mantem energizado o relay ON de um canal. |
| `gpio_actuator_disable_all_on_relays()` | Desenergiza todos os relés ON. |
| `gpio_actuator_stop_all()` | Desenergiza ON, energiza OFF por tempo definido e desenergiza OFF. |
| `gpio_actuator_get()` | Retorna status logico dos 4 canais. |
| `gpio_actuator_shutdown()` | Desenergiza todos os relés. |

## Comportamento Esperado

- No boot, todos os relés ficam desenergizados.
- Durante bombeamento, relés ON ficam energizados.
- Na parada, relés OFF sao energizados por `CONFIG_PUMP_STOP_RELAY_TIME_MS`.
- Status e calculado pela comparacao entre leitura GPIO e `status_active_level`.
