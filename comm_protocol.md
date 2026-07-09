# Protocolo de Comunicacao - Controle de Bombeamento

Este documento substitui a referencia antiga de comunicacao do pivo para o novo produto de bombeamento.

## Situacao Atual

Na imagem atual do firmware, a comunicacao serial esta ativa:

- `comm_app.c` entra no build.
- `gprs`, `rf_module` e `idp_protocol` entram no build.
- `wifi_app` e `http_server` continuam excluidos.
- RF e GPRS aceitam comandos IDP.
- RF e o modo principal padrao.

Neste firmware, `MQTT` representa o caminho serial pelo modem/GPRS UART. A placa nao conecta diretamente em broker MQTT.

## Envelope IDP

O padrao IDP continua sendo ASCII simples:

```text
#IDP-DEVICE_ID-CAMPO_1-CAMPO_2-...$
```

Regras:

- `#` inicia o pacote.
- `$` finaliza o pacote.
- `-` separa campos.
- `IDP` e numerico.
- `DEVICE_ID` identifica a placa/produto.
- Comandos devem responder com ACK, snapshot ou erro.
- Campos de status usam `0 = OFF`, `1 = ON`, `2 = UNKNOWN`.
- Campos de comando usam `0 = NONE`, `1 = ON/START`, `2 = OFF/STOP`.

## IDPs Principais do Novo Produto

| IDP | Nome | Situacao | Funcao |
| --- | --- | --- | --- |
| `0` | Status de bombeamento | Adaptado | Retorna snapshot dos 4 status digitais e estado logico da bomba. |
| `1` | Comando de bombeamento | Adaptado | Solicita partida sequencial ou parada segura. |
| `3` | Configuracao de atuacao | Adaptado | Configura parada, leitura ociosa, rampas, intervalos de partida e telemetria. |
| `13` | Exclusao de agenda | Adaptado | Exclui uma agenda por identificador. |
| `14` | Liga/desliga por data | Adaptado | Agenda partida e parada usando data absoluta ou atraso em segundos. |
| `16` | Apenas desliga por data | Adaptado | Agenda somente uma parada segura. |
| `18` | Evento de agenda | Adaptado | Publica partida, parada ou expiracao de agenda. |
| `21` | Timestamp | Reaproveitado | Sincroniza RTC. |
| `28` | Motivo de desligamento | Adaptado | Informa o ultimo desligamento salvo em NVS e publica eventos de parada/falha/boot. |
| `29` | Progresso de acionamento | Novo | Publica etapa, motor, fase e timer da partida/parada para UI local. |
| `30` | Acao local/manual | Adaptado futuro | Representa comando local ou evento manual. |
| `42` | Heartbeat de conectividade | Adaptado | Supervisiona o ESP de conectividade pela GPRS UART. |
| `90` | Versao de firmware | Reaproveitado | Retorna versao. |
| `91` | Reset da placa | Reaproveitado | Reinicia ESP32-S3. |

## IDP 0 - Status

Formato esperado:

```text
#00-DEVICE_ID-PUMP_STATE-C1-C2-C3-C4-LAST_FAULT-TIMESTAMP$
```

Campos:

| Campo | Valores esperados |
| --- | --- |
| `PUMP_STATE` | `STOPPED`, `STARTING`, `RUNNING`, `STOPPING`, `FAULT` |
| `C1..C4` | `0 = OFF`, `1 = ON`, `2 = UNKNOWN` |
| `LAST_FAULT` | `0 = sem falha`; valores textuais ou numericos podem ser definidos na etapa de comunicacao |
| `TIMESTAMP` | Unix timestamp vindo do RTC |

Na implementacao atual local, a leitura real vem de `actuation_app_get_status()` e `gpio_actuator_get()`.

## IDP 1 - Comando

Formato recomendado, alinhado ao tipo `actuation_actions`:

```text
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
```

Valores:

| Valor | Significado |
| --- | --- |
| `0` | Nenhum comando para o canal |
| `1` | Solicita partida do bombeamento |
| `2` | Solicita parada segura |

Regra do produto:

- Qualquer canal com `1` deve iniciar a sequencia de bombeamento completa.
- Qualquer canal com `2` deve solicitar parada segura.
- Se houver `ON` e `OFF` no mesmo pacote, `OFF` deve ter prioridade.

Exemplos:

```text
#01-newproduct_1-1-0-0-0-operador$
#01-newproduct_1-0-0-0-2-operador$
```

## IDP 3 - Configuracao de Atuacao

Consulta:

```text
#03-DEVICE_ID$
```

Configuracao:

```text
#03-DEVICE_ID-OFF_RELAY_MS-IDLE_READ_SEC-STATUS_ACTIVE_LEVEL-RAMP1_SEC-STAGE1_SEC-RAMP2_SEC-STAGE2_SEC-RAMP3_SEC-STAGE3_SEC-RAMP4_SEC-STAGE4_SEC-STATUS_00_MIN$
```

Campos:

| Campo | Descricao |
| --- | --- |
| `OFF_RELAY_MS` | Tempo usado para relés OFF/parada. Padrao atual: `10000`. |
| `IDLE_READ_SEC` | Cadencia interna de leitura quando parado, em segundos. Padrao atual: `10`. |
| `STATUS_ACTIVE_LEVEL` | `0 = entrada ativa em nivel baixo`; `1 = entrada ativa em nivel alto`. Padrao atual: `0`. |
| `RAMP1_SEC..RAMP4_SEC` | Tempo de rampa da softstarter de cada motor. Padrao atual e minimo seguro para `0`: `5`. |
| `STAGE1_SEC` | Intervalo apos a rampa do canal 1. Padrao atual: `10`. |
| `STAGE2_SEC` | Intervalo apos a rampa do canal 2. Padrao atual: `30`. |
| `STAGE3_SEC` | Intervalo apos a rampa do canal 3. Padrao atual: `30`. |
| `STAGE4_SEC` | Intervalo apos a rampa do canal 4, antes de `RUNNING`. Padrao atual: `0`. |
| `STATUS_00_MIN` | Intervalo de envio periodico do `#00$`, em minutos. Padrao atual: `1`. |

Exemplo:

```text
#03-newproduct_1-10000-10-0-5-10-5-30-5-30-5-0-1$
```

`IDLE_READ_SEC` e `STATUS_00_MIN` sao independentes: o primeiro controla somente a leitura local quando parado; o segundo controla a comunicacao periodica.

## IDPs 13, 14, 16 e 18 - Agendamentos

Criar partida e parada por data:

```text
#14-DEVICE_ID-START_TIME-END_TIME-USER$
```

Criar somente parada por data:

```text
#16-DEVICE_ID-END_TIME-USER$
```

Consultar:

```text
#14-DEVICE_ID$
#16-DEVICE_ID$
```

Excluir pelo identificador retornado na criacao:

```text
#13-DEVICE_ID-SCHEDULE_ID-USER$
```

Evento espontaneo:

```text
#18-DEVICE_ID-SOURCE_IDP-SCHEDULE_ID-EVENT-USER-TIMESTAMP$
```

Os campos de data aceitam Unix timestamp absoluto ou, para valores abaixo de `1000000000`, atraso em segundos a partir do RTC atual. Consulte [Agendamentos por Data](docs/functional/scheduling.md) para respostas e regras de boot.

## IDP 28 - Informacao de Desligamento

Consulta:

```text
#28-DEVICE_ID$
```

Resposta e evento:

```text
#28-DEVICE_ID-REASON-ORIGIN-USER-PHASE-MOTOR-RESET_REASON-TIMESTAMP$
```

Campos:

| Campo | Valores esperados |
| --- | --- |
| `REASON` | `command_off`, `startup_fault`, `runtime_fault`, `brownout`, `boot`, `none` |
| `ORIGIN` | `command`, `actuation_app`, `boot`, `unknown` |
| `USER` | Usuario do comando, ou `unknown`. |
| `PHASE` | `starting`, `running`, `stopped`, `was_commanded_on`, `boot`, `unknown`. |
| `MOTOR` | `0` quando nao aplicavel; `1..4` quando uma leitura de motor falhou. |
| `RESET_REASON` | `none`, `brownout`, `poweron`, `software`, `panic`, `wdt`, etc. |
| `TIMESTAMP` | Unix timestamp vindo do RTC. |

O firmware salva o pacote completo em NVS usando `DATA_TYPE_REASON_HANG_UP` / `reason_hangup`. O pacote e enviado em toda parada segura, toda falha e todo boot pelo modo principal configurado.

Se o boot ocorrer com a ultima acao persistida ainda em ON, o pacote sai com `PHASE=was_commanded_on` e `RESET_REASON` real, mesmo quando o reset nao for classificado como brownout pelo ESP-IDF.

## IDP 29 - Progresso de Acionamento

Evento espontaneo de saida usado pela interface local do ESP de conectividade. O ESP32-S3 nao aceita `#29` como comando ou consulta; se esse pacote voltar pela UART, ele e ignorado para evitar eco serial.

Durante timers longos, o progresso periodico e limitado por `CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS`, hoje `5000 ms`, para reduzir carga na UART. Eventos de inicio/fim de fase, troca de motor, `RUNNING`, `STOPPING`, `FAULT` e `STOPPED` continuam imediatos.

```text
#29-DEVICE_ID-PUMP_STATE-MOTOR-PHASE-ELAPSED_SEC-TOTAL_SEC-C1-C2-C3-C4$
```

Campos:

| Campo | Valores esperados |
| --- | --- |
| `PUMP_STATE` | `STARTING`, `RUNNING`, `STOPPING`, `FAULT`, `STOPPED` |
| `MOTOR` | `0` para evento global; `1..4` para etapa de motor. |
| `PHASE` | `START_REQUESTED`, `ON`, `RAMP`, `STAGE`, `RUNNING`, `STOPPING`, `FAULT`, `STOPPED` |
| `ELAPSED_SEC` | Tempo decorrido da fase atual, em segundos. |
| `TOTAL_SEC` | Tempo total configurado para a fase atual, em segundos. `0` quando nao aplicavel. |
| `C1..C4` | Snapshot das leituras no momento do evento. |

Exemplo:

```text
#29-newproduct_1-STARTING-2-RAMP-3-5-1-1-0-0$
```

## IDP 42 - Heartbeat de Conectividade

O ESP32-S3 envia a cada 30 segundos pela GPRS UART:

```text
#42-DEVICE_ID-PING$
```

O ESP de conectividade deve responder:

```text
#42-DEVICE_ID-PONG$
```

Consulta do estado visto pela placa:

```text
#42-DEVICE_ID$
```

A resposta e `#42-DEVICE_ID-ALIVE$` ou `#42-DEVICE_ID-NO_HEARTBEAT$`. A ausencia de resposta por 90 segundos gera warning, sem reset fisico: esta placa nao controla alimentacao nem reset do ESP de conectividade.

## Comunicacao MQTT, RF e HTTP

RF e GPRS UART estao ativos. HTTP permanece fora do build atual.

| Canal | Componente | Papel esperado |
| --- | --- | --- |
| MQTT via ESP de conectividade | `gprs` + `comm_app` | Receber pacotes IDP pela GPRS UART e enviar respostas/eventos. |
| RF | `rf_module` + `comm_app` | Receber/enviar pacotes IDP brutos por UART RF. |
| HTTP local | `wifi_app` + `http_server` + `comm_app` | Fora do build atual. |

Respostas de comando voltam pela UART que recebeu o pacote. Eventos espontaneos, como mudanca de status, usam o modo principal salvo em NVS. O padrao de fabrica e `RF`.

Broker, topicos, autenticacao, QoS, ACK e retry pertencem ao firmware do ESP de conectividade. IDPs 2 e 6 permanecem sem handler nesta placa ate existir um contrato de configuracao entre os dois firmwares. IDPs 24 e 92 foram removidos e respondem como nao suportados.

## IDP 31 - Modo Principal

Consulta:

```text
#31-newproduct_1$
```

Configurar RF:

```text
#31-newproduct_1-RF$
```

Configurar GPRS/MQTT:

```text
#31-newproduct_1-MQTT$
```

## IDPs Removidos da Regra Principal

Os IDPs ligados a pivo, angulo, barreira, setor, rush mode, GPS e agenda por angulo nao fazem parte da regra principal do produto de bombeamento.

Detalhamento completo: [Levantamento de IDPs](docs/newproduct_1_idp_migration.md).
