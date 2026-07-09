# Componente `gprs`

## Situacao

Ativo no build atual.

## Funcao

Comunicar a placa por UART com outro ESP32, responsavel pela conectividade e pelo MQTT.

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
4. Envia respostas/eventos para o ESP de conectividade.
5. Transporta o heartbeat IDP 42.

## Observacao

O componente trafega pacotes IDP brutos. `COMM_MQTT` identifica o caminho UART, nao uma conexao MQTT local. Broker, topicos, autenticacao, QoS e retry pertencem ao firmware do ESP de conectividade.

Nao existe GPIO para reset fisico desse ESP neste firmware. Timeout de heartbeat apenas gera warning.
