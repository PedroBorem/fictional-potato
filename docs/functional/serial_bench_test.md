# Teste Serial de Bancada

## Objetivo

Testar a comunicacao IDP usando um conversor USB-serial/FTDI, sem modem GPRS nem modulo RF conectados.

## UARTs Disponiveis

| Caminho | Baud rate | TX da placa | RX da placa |
| --- | --- | --- | --- |
| RF | `9600` | `GPIO_NUM_2` | `GPIO_NUM_1` |
| GPRS/MQTT | `115200` | `GPIO_NUM_17` | `GPIO_NUM_18` |

RF e o modo principal padrao. A placa aceita comando nas duas UARTs.

## Ligacao com FTDI

Para testar RF:

| Placa | FTDI |
| --- | --- |
| `GPIO_NUM_2` TX | RX |
| `GPIO_NUM_1` RX | TX |
| GND | GND |

Para testar GPRS:

| Placa | FTDI |
| --- | --- |
| `GPIO_NUM_17` TX | RX |
| `GPIO_NUM_18` RX | TX |
| GND | GND |

Use FTDI em nivel logico de 3.3V. Nao aplique 5V em GPIO do ESP32-S3.

## Pacotes para Teste

O firmware processa pacotes completos entre `#` e `$`. Quebras de linha `CR/LF` sao ignoradas, e varios pacotes podem chegar no mesmo evento UART.

Consultar status:

```text
#00-new_product$
```

Iniciar bombeamento:

```text
#01-new_product-1-0-0-0-bancada$
```

Parar bombeamento:

```text
#01-new_product-0-0-0-2-bancada$
```

Consultar modo principal:

```text
#31-new_product$
```

Configurar RF como principal:

```text
#31-new_product-RF$
```

Configurar GPRS/MQTT como principal:

```text
#31-new_product-MQTT$
```

Consultar versao:

```text
#90-new_product$
```

Consultar configuracao de atuacao:

```text
#03-new_product$
```

Configurar atuacao, intervalos de partida `10-30-30` e envio periodico de `#00$` a cada 60 segundos:

```text
#03-new_product-10000-10-0-10-30-30-60$
```

## Respostas Esperadas

Comando aceito:

```text
#01-new_product-ACCEPTED$
```

Status:

```text
#00-new_product-STATE-C1-C2-C3-C4-LAST_FAULT-TIMESTAMP$
```

Exemplo:

```text
#00-new_product-RUNNING-1-1-1-1-0-1780000000$
```

## Comportamento dos Canais

- Resposta direta volta pela UART que recebeu o comando.
- Eventos espontaneos usam o modo principal salvo em NVS.
- O padrao de fabrica e `RF`.
- `MQTT` significa GPRS UART neste firmware.

## Cenario de Bancada com Relés e Leituras

No cenario descrito de bancada, o acionamento do relay ON deve alimentar a entrada do optoacoplador correspondente.

Como o firmware interpreta leitura ativa em nivel baixo, confirme eletricamente que a saida logica do opto para o ESP32-S3 fica em `0` quando o relé de liga estiver acionado. Se a saida logica ficar em `1`, o firmware entendera a leitura como OFF e executara parada por falha.
