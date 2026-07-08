# Componente `gprs`

## Situacao

Ativo no build atual.

## Funcao Preservada

Comunicar a placa com modem externo por UART, historicamente usado para trafego MQTT.

## Configuracao

| Item | Valor |
| --- | --- |
| UART | `UART_NUM_1` |
| TX | `GPIO_NUM_17` |
| RX | `GPIO_NUM_18` |
| Baud rate | `115200` |

## Fluxo Atual

1. Recebe bytes pela UART.
2. Remove caracteres de controle.
3. Entrega payload para callback como `COMM_MQTT`.
4. Envia respostas/eventos para o modem.

## Observacao

O componente deve continuar trafegando pacotes IDP brutos. A interpretacao de comando de bombeamento deve ficar fora do driver UART, na camada de comunicacao/aplicacao.
