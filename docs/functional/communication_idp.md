# Comunicacao e Padrao IDP

## Status de Implementacao

A comunicacao serial esta ativa no firmware atual. O produto aceita IDPs por RF UART e GPRS UART. HTTP/app e Wi-Fi permanecem fora desta etapa.

Atualmente:

- `comm_app.c` e compilado.
- `gprs`, `rf_module` e `idp_protocol` sao compilados.
- `wifi_app` e `http_server` estao em `EXCLUDE_COMPONENTS`.
- RF e o modo principal padrao.

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
#00-new_product$
```

### IDP 1 - Comando

```text
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
```

Uso esperado:

- `#01-new_product-1-0-0-0-app$` inicia partida sequencial.
- `#01-new_product-0-0-0-2-app$` solicita parada segura.

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
- Manter a leitura interna ociosa em segundos e a telemetria em minutos.

Exemplo:

```text
#03-new_product-10000-10-0-5-10-5-30-5-30-5-0-1$
```

Ordem dos campos temporais por motor: `RAMP1-STAGE1-RAMP2-STAGE2-RAMP3-STAGE3-RAMP4-STAGE4`.

`IDLE_READ_SEC` nao e o mesmo timer de `STATUS_00_MIN`. Eles podem receber valores equivalentes por decisao de configuracao, mas possuem responsabilidades diferentes.

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

### IDP 31 - Modo Principal de Comunicacao

```text
#31-DEVICE_ID-MODE$
```

Valores:

- `RF`
- `MQTT`

Neste firmware, `MQTT` significa caminho serial via GPRS UART. RF e o padrao.

## Canais

### MQTT via modem

O componente `gprs` recebe pacotes pela UART do modem e entrega para callback como `COMM_MQTT`.

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
