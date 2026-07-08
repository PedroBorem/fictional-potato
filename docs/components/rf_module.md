# Componente `rf_module`

## Situacao

Ativo no build atual.

## Funcao Preservada

Comunicar a placa por UART com modulo RF/LoraMesh.

## Configuracao

| Item | Valor |
| --- | --- |
| UART | `UART_NUM_2` |
| TX | `GPIO_NUM_2` |
| RX | `GPIO_NUM_1` |
| Baud rate | `9600` |

## Fluxo Atual

1. Recebe bytes pela UART RF.
2. Filtra caracteres invalidos.
3. Entrega payload para callback como `COMM_RF`.
4. Envia payloads IDP por UART.

## Observacao

RF e o canal principal padrao para eventos espontaneos do produto.
