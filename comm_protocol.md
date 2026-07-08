# Protocolo de Comunicacao - Controle de Bombeamento

Este documento substitui a referencia antiga de comunicacao do pivo para o novo produto de bombeamento.

## Situacao Atual

Na imagem atual do firmware, a comunicacao externa esta desabilitada:

- `comm_app.c` nao entra no build do componente `applications`.
- `gprs`, `rf_module`, `wifi_app`, `http_server` e `idp_protocol` estao excluidos no `CMakeLists.txt` raiz.
- O funcionamento local do bombeamento esta ativo e independente de HTTP, MQTT ou RF.

O contrato abaixo define o padrao esperado para a segunda etapa, quando a comunicacao for reativada.

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
| `3` | Configuracao de atuacao | Adaptado | Configura tempo de pulso/parada, intervalo de leitura e nivel ativo das entradas. |
| `21` | Timestamp | Reaproveitado | Sincroniza RTC. |
| `28` | Motivo de parada/falha | Adaptado futuro | Informa ultima falha ou parada segura. |
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
#01-bomba_1-1-0-0-0-operador$
#01-bomba_1-0-0-0-2-operador$
```

## IDP 3 - Configuracao de Atuacao

Formato recomendado:

```text
#03-DEVICE_ID-RELAY_PULSE_MS-READ_TIME_SEC-STATUS_ACTIVE_LEVEL$
```

Campos:

| Campo | Descricao |
| --- | --- |
| `RELAY_PULSE_MS` | Tempo usado para relés OFF/parada. Padrao atual: `10000`. |
| `READ_TIME_SEC` | Intervalo de leitura periodica quando parado. Padrao atual: `10`. |
| `STATUS_ACTIVE_LEVEL` | `0 = entrada ativa em nivel baixo`; `1 = entrada ativa em nivel alto`. Padrao atual: `0`. |

Observacao: a partida da bomba usa tempos fixos de sequencia definidos em `project_config.h`: 10s, 30s, 30s.

## Comunicacao MQTT, RF e HTTP

Esses canais estao fora do build atual. Quando forem reativados, o padrao esperado e:

| Canal | Componente | Papel esperado |
| --- | --- | --- |
| MQTT via modem | `gprs` + `comm_app` | Receber pacotes IDP do modem e enviar respostas/eventos para a nuvem. |
| RF | `rf_module` + `comm_app` | Receber/enviar pacotes IDP brutos por UART RF. |
| HTTP local | `wifi_app` + `http_server` + `comm_app` | Expor API local para configuracao e comandos durante instalacao/manutencao. |

Enquanto a segunda etapa nao for implementada, comandos de partida/parada devem entrar diretamente pela camada de aplicacao, chamando `actuation_app_set_actions()`.

## IDPs Removidos da Regra Principal

Os IDPs ligados a pivo, angulo, barreira, setor, rush mode, GPS e agenda por angulo nao fazem parte da regra principal do produto de bombeamento.

Detalhamento completo: [Levantamento de IDPs](docs/new_product_idp_migration.md).
