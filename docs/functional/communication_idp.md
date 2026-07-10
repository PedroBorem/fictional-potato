# Comunicacao e Padrao IDP

## Status de Implementacao

A comunicacao serial esta ativa no firmware atual. O produto aceita IDPs por RF UART e GPRS UART. HTTP/app e Wi-Fi permanecem fora desta etapa.

Atualmente:

- `comm_app.c` e compilado.
- `gprs`, `rf_module` e `idp_protocol` sao compilados.
- `wifi_app` e `http_server` estao em `EXCLUDE_COMPONENTS`.
- RF e o modo principal padrao.
- O ESP32-S3 nao acessa broker MQTT e nao controla reset ou alimentacao do ESP de conectividade.

## Arquitetura Atual

```text
GPRS UART / RF UART
    |
comm_app
    |
idp_protocol
    |
system_manager ou handler de comandos
    |
actuation_app
    |
gpio_actuator
```

## Envelope

```text
#IDP-DEVICE_ID-CAMPO_1-CAMPO_2-...$
```

Campos:

- `IDP`: identificador numerico.
- `DEVICE_ID`: identificacao logica do produto.
- Campos posteriores: payload do comando, configuracao ou telemetria.

## Padroes de Valor

Status de canal:

| Valor | Significado |
| --- | --- |
| `0` | OFF |
| `1` | ON |
| `2` | UNKNOWN |

Comando de canal:

| Valor | Significado |
| --- | --- |
| `0` | NONE |
| `1` | START / ON |
| `2` | STOP / OFF |

## Pacotes Recomendados

### IDP 0 - Status

```text
#00-DEVICE_ID-PUMP_STATE-C1-C2-C3-C4-LAST_FAULT-TIMESTAMP$
```

Uso esperado:

- Consulta de estado.
- Publicacao apos mudanca de leitura.
- Publicacao apos partida, parada ou falha.

Exemplo:

```text
#00-DEVICE_ID$
```

### IDP 1 - Comando

```text
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
```

Uso esperado:

- `#01-DEVICE_ID-1-0-0-0-app$` inicia partida sequencial.
- `#01-DEVICE_ID-0-0-0-2-app$` solicita parada segura.

Mesmo existindo 4 campos de comando, a regra de negocio e global: qualquer `1` liga a bomba completa e qualquer `2` para tudo.

### IDP 3 - Configuracao

Consulta:

```text
#03-DEVICE_ID$
```

Configuracao:

```text
#03-DEVICE_ID-OFF_RELAY_MS-IDLE_READ_SEC-STATUS_ACTIVE_LEVEL-RAMP1_SEC-STAGE1_SEC-RAMP2_SEC-STAGE2_SEC-RAMP3_SEC-STAGE3_SEC-RAMP4_SEC-STAGE4_SEC-STATUS_00_MIN$
```

Uso esperado:

- Ajustar parametros de atuacao.
- Persistir `act_config` em NVS.
- Reaplicar configuracao no `gpio_actuator`.
- Configurar rampas da softstarter, intervalos de partida e envio periodico de `#00$`.
- Configurar os intervalos da parada one-wire: cada `RAMPx_SEC` e reutilizado depois que o relé ON correspondente e desenergizado.
- Manter a leitura interna ociosa em segundos e a telemetria em minutos.

`OFF_RELAY_MS` permanece no pacote somente por compatibilidade. O firmware atual nao energiza relés OFF.

Exemplo:

```text
#03-DEVICE_ID-10000-10-0-5-10-5-30-5-30-5-0-1$
```

Ordem dos campos temporais por motor: `RAMP1-STAGE1-RAMP2-STAGE2-RAMP3-STAGE3-RAMP4-STAGE4`.

`IDLE_READ_SEC` nao e o mesmo timer de `STATUS_00_MIN`. Eles podem receber valores equivalentes por decisao de configuracao, mas possuem responsabilidades diferentes.

### IDP 6 - Identidade Persistente

```text
#06-DEVICE_ID$
#06-DEVICE_ID-NOVO_DEVICE_ID$
```

O primeiro formato consulta a identidade atual. O segundo troca o ID persistido em NVS e responde `#06-NOVO_DEVICE_ID$`. A troca exige que o primeiro campo corresponda ao ID atual e aceita apenas letras, numeros e `_` no novo valor. O ID de recuperacao de fabrica so e usado quando a NVS nao possui um valor valido.

### IDP 28 - Informacao de Desligamento

```text
#28-DEVICE_ID-REASON-ORIGIN-USER-PHASE-MOTOR-RESET_REASON-TIMESTAMP$
```

Consulta:

```text
#28-DEVICE_ID$
```

Uso esperado:

- Retornar o ultimo desligamento salvo em NVS.
- Publicar evento quando houver comando de parada.
- Publicar evento quando houver falha durante partida ou operacao.
- Publicar evento em todo boot; se o reset foi brownout, o motivo salvo passa a ser `brownout`.
- Publicar evento de boot com `PHASE=was_commanded_on` quando a ultima acao persistida ainda era ON.

Motivos implementados:

| `REASON` | Uso |
| --- | --- |
| `command_off` | Parada por comando OFF, com `USER` do IDP 1. |
| `startup_fault` | Falha antes de entrar em `RUNNING`. |
| `runtime_fault` | Falha apos partida concluida. |
| `brownout` | Boot apos reset por brownout. |
| `boot` | Primeiro boot sem motivo anterior salvo. |
| `none` | Sem motivo conhecido. |

Se uma falha for de leitura de motor, `MOTOR` informa o canal `1..4`.

### IDP 29 - Progresso de Acionamento

Evento espontaneo de saida. Nao deve ser enviado como comando ou consulta para o ESP32-S3.

Durante rampas e intervalos, o envio periodico e limitado a `CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS`, hoje `5000 ms`, para reduzir trafego na UART.

```text
#29-DEVICE_ID-PUMP_STATE-MOTOR-PHASE-ELAPSED_SEC-TOTAL_SEC-C1-C2-C3-C4$
```

Uso esperado:

- Alimentar a UI local do ESP de conectividade durante a partida.
- Mostrar qual motor esta em `ON`, `RAMP` ou `STAGE`.
- Mostrar `ELAPSED_SEC/TOTAL_SEC` para barra de progresso.
- Publicar `FAULT`/`STOPPING`/`STOPPED` nos eventos de parada.

Exemplo:

```text
#29-DEVICE_ID-STARTING-1-RAMP-4-5-1-0-0-0$
```

### IDPs 13, 14, 16 e 18 - Agendamentos

```text
#14-DEVICE_ID-START_TIME-END_TIME-USER$
#16-DEVICE_ID-END_TIME-USER$
#13-DEVICE_ID-SCHEDULE_ID-USER$
#18-DEVICE_ID-SOURCE_IDP-SCHEDULE_ID-EVENT-USER-TIMESTAMP$
```

IDP 14 agenda a partida sequencial completa e sua parada. IDP 16 agenda apenas a parada segura. IDP 13 exclui uma agenda e IDP 18 publica os eventos `STARTED`, `STOPPED` e `EXPIRED`.

Datas podem ser Unix timestamp ou atraso em segundos. O contrato completo esta em [Agendamentos por Data](scheduling.md).

### IDP 12 - Historico de Bombeamento

Consulta:

```text
#12-DEVICE_ID$
```

Resposta por registro:

```text
#12-DEVICE_ID-INDEX-START_TS-END_TS-USER-START_REASON-STOP_REASON-STOP_PHASE-MOTOR-RESET_REASON-EVENT_TS$
```

Sem historico:

```text
#12-DEVICE_ID-NONE$
```

O historico une comando, status e desligamento:

- `START_TS`, `USER` e `START_REASON` representam o comando de liga aceito.
- `START_REASON=command` para IDP 1 manual/web/MQTT/RF.
- `START_REASON=schedule_14_*` quando a partida veio de agenda.
- `END_TS`, `STOP_REASON`, `STOP_PHASE`, `MOTOR`, `RESET_REASON` e `EVENT_TS` representam desligamento ou falha.
- Falhas registram o momento (`starting_ramp`, `starting_stage`, `running`) e o motor `1..4`.

O firmware salva ate 20 registros em NVS na chave `history`.

### IDP 42 - Heartbeat

```text
#42-DEVICE_ID-PING$
#42-DEVICE_ID-PONG$
#42-DEVICE_ID$
```

A placa envia `PING` a cada 30 segundos somente na GPRS UART e considera timeout apos 90 segundos sem `PING` ou `PONG`. O timeout gera log, mas nao reinicia nenhum equipamento.

### IDP 31 - Modo Principal de Comunicacao

```text
#31-DEVICE_ID-MODE$
```

Valores:

- `RF`
- `MQTT`

Neste firmware, `MQTT` significa caminho serial via GPRS UART. RF e o padrao.

## Canais

### MQTT via ESP de conectividade

O componente `gprs` recebe pacotes IDP pela UART do ESP de conectividade e entrega para callback como `COMM_MQTT`. O nome representa a origem/destino do transporte; MQTT, broker e topicos nao existem neste firmware.

### RF

O componente `rf_module` recebe pacotes pela UART RF e entrega para callback como `COMM_RF`.

### HTTP local

HTTP local esta fora do build atual.

## Regras de Integracao

- A comunicacao nao deve acionar GPIO diretamente.
- Todo comando de bombeamento deve passar por `actuation_app_set_actions()`.
- Todo status deve sair de `actuation_app_get_status()`.
- O IDP deve representar a regra nova de bombeamento, sem campos de angulo, percentimetro, setor, barreira ou sentido de giro.
- Em caso de comando simultaneo de ligar e desligar, desligar tem prioridade.
- Respostas voltam pelo canal de origem; eventos espontaneos usam o canal principal configurado no IDP 31.
- IDPs 2 e 6 aguardam contrato entre os dois firmwares.
- IDPs 24 e 92 nao existem no novo produto e respondem `unsupported_idp`.
