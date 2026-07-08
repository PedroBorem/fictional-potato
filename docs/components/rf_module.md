# Componente `rf_module`

## Situacao

Inativo no build atual.

Esta listado em `EXCLUDE_COMPONENTS` no `CMakeLists.txt` raiz.

## Funcao Preservada

Comunicar a placa por UART com modulo RF/LoraMesh.

## Configuracao

| Item | Valor |
| --- | --- |
| UART | `UART_NUM_2` |
| TX | `GPIO_NUM_2` |
| RX | `GPIO_NUM_1` |
| Baud rate | `9600` |

## Fluxo Esperado Quando Reativado

1. Recebe bytes pela UART RF.
2. Filtra caracteres invalidos.
3. Entrega payload para callback como `COMM_RF`.
4. Envia payloads IDP por UART.

## Observacao

RF pode ser reaproveitado como canal secundario de comandos/status, desde que use os IDPs novos de bombeamento e nao os campos antigos de pivo.
