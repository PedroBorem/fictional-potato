# Padrao de Logs e Cores

## Formato

Logs categorizados seguem:

```text
[ORIGEM][TIMESTAMP ms] mensagem
```

`TIMESTAMP` e o tempo monotonico em milissegundos desde o boot. Ele funciona antes da inicializacao do RTC e nao sofre saltos quando a data/hora e ajustada.

Exemplos:

```text
[UART_GPRS][9404 ms] RX #01-newproductteste_1-1-0-0-0-app$
[TIMER][10414 ms] Stage 1/4 ramp heartbeat ...... 1/5 s
[NVS][19424 ms] nvs_data_set, saved successfully
```

## Paleta do Projeto

| Categoria | Cor | ANSI | Uso |
| --- | --- | --- | --- |
| Erro | Vermelho | `31` | Falhas de hardware, protocolo ou operacao. |
| Warning | Amarelo | `33` | Situacoes anormais recuperaveis. |
| NVS | Branco | `37` | Leitura, escrita e inicializacao normal da NVS. |
| Timer | Ciano | `36` | Rampas, intervalos e heartbeat dos timers. |
| Padrao | Verde | `32` | Status e operacao normal. |
| Processamento | Azul | `34` | Etapas, comandos e transicoes em execucao. |
| UART GPRS | Roxo/magenta | `35` | RX e TX da UART GPRS/MQTT. |
| UART RF | Ciano brilhante | `96` | RX e TX da UART RF. |
| Sucesso | Verde brilhante | `92` | Inicializacao e conclusao bem-sucedida. |
| Debug | Cinza | `90` | Diagnostico oculto no nivel INFO padrao. |

Erros e warnings têm prioridade sobre a cor da origem. Assim, um erro da UART GPRS continua vermelho, mas mantém `[UART_GPRS]`.

## Cores ANSI Disponiveis

| Codigo | Cor | Codigo brilhante | Cor brilhante |
| --- | --- | --- | --- |
| `30` | Preto | `90` | Cinza |
| `31` | Vermelho | `91` | Vermelho brilhante |
| `32` | Verde | `92` | Verde brilhante |
| `33` | Amarelo | `93` | Amarelo brilhante |
| `34` | Azul | `94` | Azul brilhante |
| `35` | Magenta | `95` | Magenta brilhante |
| `36` | Ciano | `96` | Ciano brilhante |
| `37` | Branco | `97` | Branco brilhante |

As cores dependem de `CONFIG_LOG_COLORS=y`, ja habilitado em `sdkconfig.defaults`.
