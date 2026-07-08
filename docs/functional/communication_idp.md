# Comunicacao e Padrao IDP

## Status de Implementacao

A comunicacao esta intencionalmente fora do firmware atual. O produto local ja funciona com a nova regra de bombeamento, mas HTTP/app, MQTT e RF serao trabalhados em uma segunda etapa.

Atualmente:

- `comm_app.c` nao e compilado.
- `gprs`, `rf_module`, `wifi_app`, `http_server` e `idp_protocol` estao em `EXCLUDE_COMPONENTS`.
- O parser IDP antigo permanece no repositorio para reaproveitamento.

## Arquitetura Esperada para a Segunda Etapa

```text
HTTP/MQTT/RF
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

### IDP 1 - Comando

```text
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
```

Uso esperado:

- `#01-bomba_1-1-0-0-0-app$` inicia partida sequencial.
- `#01-bomba_1-0-0-0-2-app$` solicita parada segura.

Mesmo existindo 4 campos de comando, a regra de negocio e global: qualquer `1` liga a bomba completa e qualquer `2` para tudo.

### IDP 3 - Configuracao

```text
#03-DEVICE_ID-RELAY_PULSE_MS-READ_TIME_SEC-STATUS_ACTIVE_LEVEL$
```

Uso esperado:

- Ajustar parametros de atuacao.
- Persistir `act_config` em NVS.
- Reaplicar configuracao no `gpio_actuator`.

### IDP 28 - Ultima Falha

```text
#28-DEVICE_ID-LAST_FAULT-CHANNEL-TIMESTAMP$
```

Uso esperado futuro:

- Diagnostico de parada por falha de leitura.
- Historico resumido para nuvem/app.

## Canais Futuros

### MQTT via modem

O componente `gprs` recebe pacotes pela UART do modem e entrega para callback como `COMM_MQTT`.

### RF

O componente `rf_module` recebe pacotes pela UART RF e entrega para callback como `COMM_RF`.

### HTTP local

O componente `http_server` recebe payloads HTTP e entrega para callback como `COMM_HTTP_GET` ou `COMM_HTTP_POST`.

## Regras de Integracao

- A comunicacao nao deve acionar GPIO diretamente.
- Todo comando de bombeamento deve passar por `actuation_app_set_actions()`.
- Todo status deve sair de `actuation_app_get_status()`.
- O IDP deve representar a regra nova de bombeamento, sem campos de angulo, percentimetro, setor, barreira ou sentido de giro.
- Em caso de comando simultaneo de ligar e desligar, desligar tem prioridade.
