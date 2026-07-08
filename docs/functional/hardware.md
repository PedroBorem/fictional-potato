# Hardware e Pinagem

## Placa Alvo

- MCU: ESP32-S3.
- ESP-IDF: v6.0.2.
- Flash configurada no firmware: 16MB.
- Relés: placa de 8 canais organizada em 4 pares ON/OFF.
- Entradas: 4 leituras digitais de status.

## Relés

| Canal | Funcao | GPIO |
| --- | --- | --- |
| 1 | Relay ON | `GPIO_NUM_13` |
| 1 | Relay OFF | `GPIO_NUM_12` |
| 2 | Relay ON | `GPIO_NUM_11` |
| 2 | Relay OFF | `GPIO_NUM_10` |
| 3 | Relay ON | `GPIO_NUM_9` |
| 3 | Relay OFF | `GPIO_NUM_8` |
| 4 | Relay ON | `GPIO_NUM_16` |
| 4 | Relay OFF | `GPIO_NUM_35` |

Os relés sao ativos em nivel baixo:

| Nivel GPIO | Estado do relé |
| --- | --- |
| `0` | Energizado |
| `1` | Desenergizado |

Na inicializacao, todos os relés sao colocados em estado desenergizado.

## Entradas de Status

| Canal | GPIO |
| --- | --- |
| 1 | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_4` |

As entradas sao interpretadas como ativas em nivel baixo:

| Nivel GPIO | Status logico |
| --- | --- |
| `0` | ON |
| `1` | OFF |

O nivel ativo padrao fica em `CONFIG_ACTUATION_DEFAULT_STATUS_ACTIVE_LEVEL`.

## RTC

O RTC externo DS3231 usa I2C:

| Sinal | GPIO |
| --- | --- |
| SDA | `GPIO_NUM_36` |
| SCL | `GPIO_NUM_37` |

## Comunicacao Serial

| Canal | Componente | UART/GPIO |
| --- | --- | --- |
| Modem/GPRS | `gprs` | `UART_NUM_1`, TX `GPIO_NUM_17`, RX `GPIO_NUM_18`, 115200 bps |
| RF | `rf_module` | `UART_NUM_2`, TX `GPIO_NUM_2`, RX `GPIO_NUM_1`, 9600 bps |

RF e GPRS UART entram no build atual. Wi-Fi local permanece fora do build.
