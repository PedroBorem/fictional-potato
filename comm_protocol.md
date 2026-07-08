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
| `21` | Timestamp | Reaproveitado | Sincroniza RTC. |
| `28` | Motivo de desligamento | Adaptado | Informa o ultimo desligamento salvo em NVS e publica eventos de parada/falha/boot. |
| `30` | Acao local/manual | Adaptado futuro | Representa comando local ou evento manual. |
| `42` | Heartbeat modem/placa | Reaproveitado futuro | Mantem supervisao do link serial com modem. |
| `90` | Versao de firmware | Reaproveitado | Retorna versao. |
| `91` | Reset da placa | Reaproveitado | Reinicia ESP32-S3. |
| `92` | Reset do modem / ACK generico | Reaproveitado futuro | Mantem compatibilidade com modem externo. |

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
#01-new_product-1-0-0-0-operador$
#01-new_product-0-0-0-2-operador$
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
| `RAMP1_SEC..RAMP4_SEC` | Tempo de rampa da softstarter de cada motor. Padrao atual: `0`. |
| `STAGE1_SEC` | Intervalo apos a rampa do canal 1. Padrao atual: `10`. |
| `STAGE2_SEC` | Intervalo apos a rampa do canal 2. Padrao atual: `30`. |
| `STAGE3_SEC` | Intervalo apos a rampa do canal 3. Padrao atual: `30`. |
| `STAGE4_SEC` | Intervalo apos a rampa do canal 4, antes de `RUNNING`. Padrao atual: `0`. |
| `STATUS_00_MIN` | Intervalo de envio periodico do `#00$`, em minutos. Padrao atual: `1`. |

Exemplo:

```text
#03-new_product-10000-10-0-5-10-5-30-5-30-5-0-1$
```

`IDLE_READ_SEC` e `STATUS_00_MIN` sao independentes: o primeiro controla somente a leitura local quando parado; o segundo controla a comunicacao periodica.

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

## Comunicacao MQTT, RF e HTTP

RF e GPRS UART estao ativos. HTTP permanece fora do build atual.

| Canal | Componente | Papel esperado |
| --- | --- | --- |
| MQTT via modem | `gprs` + `comm_app` | Receber pacotes IDP por UART do modem e enviar respostas/eventos. |
| RF | `rf_module` + `comm_app` | Receber/enviar pacotes IDP brutos por UART RF. |
| HTTP local | `wifi_app` + `http_server` + `comm_app` | Fora do build atual. |

Respostas de comando voltam pela UART que recebeu o pacote. Eventos espontaneos, como mudanca de status, usam o modo principal salvo em NVS. O padrao de fabrica e `RF`.

## IDP 31 - Modo Principal

Consulta:

```text
#31-new_product$
```

Configurar RF:

```text
#31-new_product-RF$
```

Configurar GPRS/MQTT:

```text
#31-new_product-MQTT$
```

## IDPs Removidos da Regra Principal

Os IDPs ligados a pivo, angulo, barreira, setor, rush mode, GPS e agenda por angulo nao fazem parte da regra principal do produto de bombeamento.

Detalhamento completo: [Levantamento de IDPs](docs/new_product_idp_migration.md).
