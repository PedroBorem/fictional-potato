# Novo Produto - Levantamento de IDPs

## Escopo

O firmware deixou de inicializar a regra de negocio de pivo. Nesta fase inicializa RF UART, GPRS UART e parser IDP. HTTP/app e Wi-Fi continuam fora do build.

Este documento classifica os IDPs que podem ser reaproveitados, os que precisam ser adaptados e os que devem ser inutilizados para o produto de bombeamento.

## Produto Atual

| Canal | Relay ON | Relay OFF | Leitura de status |
| --- | --- | --- | --- |
| 1 | `GPIO_NUM_13` | `GPIO_NUM_12` | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_11` | `GPIO_NUM_10` | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_9` | `GPIO_NUM_8` | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_16` | `GPIO_NUM_35` | `GPIO_NUM_4` |

## Funcionamento Principal

Comando de liga:

1. Liga canal 1, espera 10s e valida leitura 1.
2. Liga canal 2, espera 30s e valida leituras 1 e 2.
3. Liga canal 3, espera 30s e valida leituras 1, 2 e 3.
4. Liga canal 4 e passa a monitorar as 4 leituras.

Falha:

- Desliga todos os relés ON.
- Aciona todos os relés OFF por 10s.
- Registra estado de falha na aplicacao.

## IDPs que Podem ser Aproveitados

| IDP | Uso antigo | Como aproveitar |
| --- | --- | --- |
| `2` | Configuracao de rede da placa | Manter para fase de comunicacao se Wi-Fi/GPRS continuarem no produto. |
| `6` | Identificacao/configuracao de modem | Manter se o modem continuar usando o mesmo fluxo de identificacao. |
| `21` | Sincronizacao de timestamp | Aproveitar para RTC, logs e eventos. |
| `24` | Reboot automatico | Aproveitar como configuracao geral da placa. |
| `31` | Modo principal de comunicacao | Aproveitado para alternar eventos espontaneos entre `RF` e `MQTT` via GPRS UART. |
| `42` | Heartbeat modem/placa | Aproveitar se o modem continuar por UART. |
| `90` | Versao de firmware | Aproveitar sem mudanca conceitual. |
| `91` | Reset da placa | Aproveitar sem mudanca conceitual. |
| `92` | Reset do modem / ACK generico | Aproveitar se o modem continuar no hardware final. |
| `99` | Erro tecnico em referencias legadas | Usado como resposta textual de erro no callback atual, mesmo sem enum dedicado. |

## IDPs que Devem ser Adaptados

| IDP | Uso antigo | Adaptacao recomendada |
| --- | --- | --- |
| `0` | Leitura do estado atual do pivo | Virar snapshot do bombeamento: estado, 4 status, ultima falha e timestamp. |
| `1` | Escrita das actions do pivo | Virar comando de partida/parada do bombeamento. |
| `3` | Configuracao geral do pivo | Virar configuracao de atuacao: tempo de parada, intervalo de leitura e nivel ativo. |
| `12` | Historico do pivo | Adaptar apenas se o produto precisar de historico de comandos/falhas. |
| `13` a `18` | Agendamentos por data/angulo | Adaptar somente se houver agenda de ligar/desligar bomba por horario; remover angulo. |
| `27` | Dump de agendamentos | Adaptar junto com novos agendamentos, se existirem. |
| `28` | Motivo de desligamento do pivo | Adaptar para motivo de parada/falha de bombeamento. |
| `30` | Acao manual/local do pivo | Adaptar para comando local ou evento manual do bombeamento. |

## IDPs Inutilizados para Este Produto

| IDP | Motivo |
| --- | --- |
| `4` | Rush mode e janela ECO sao regra de pivo. |
| `5` | Setorizacao por angulo nao existe no bombeamento atual. |
| `7` | Angulo, pressao e GPS do pivo nao fazem parte dos 4 acionamentos ON/OFF. |
| `22` | Barreira fisica do pivo nao existe no novo produto. |
| `23` | GPS/centro do pivo nao faz parte do produto atual. |
| `26` | Barreira virtual do pivo nao existe no novo produto. |
| `32` | Evento de rush mode deixa de existir. |
| `34`, `40`, `41` | Pluviometro nao faz parte do escopo atual. |
| `69` | Suite de teste de hardware nao faz parte da regra principal. |

## Padrao de Payload Recomendado

### `IDP 0` - Status

```text
#00-DEVICE_ID-PUMP_STATE-C1-C2-C3-C4-LAST_FAULT-TIMESTAMP$
```

### `IDP 1` - Comando

```text
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
```

Regra:

- Qualquer `CMD_Cx = 1` inicia a sequencia completa.
- Qualquer `CMD_Cx = 2` solicita parada segura.
- Parada tem prioridade sobre partida.

### `IDP 3` - Configuracao

```text
#03-DEVICE_ID-RELAY_PULSE_MS-READ_TIME_SEC-STATUS_ACTIVE_LEVEL$
```

## Situacao no Codigo

- `main/CMakeLists.txt` compila apenas `main.c` e `system_manager.c`.
- `system_manager.c` inicializa RTC, NVS, atuacao local, comunicacao serial e handlers IDP.
- `components/applications/CMakeLists.txt` compila `rtc_app.c`, `data_app.c`, `actuation_app.c` e `comm_app.c`.
- `gprs`, `rf_module` e `idp_protocol` entram no build atual.
- `wifi_app` e `http_server` continuam excluidos no `CMakeLists.txt` raiz.
- Os dados NVS novos sao `act_actions` e `act_config`.
- O modo principal de comunicacao fica em `comm_main_mode`, com `RF` como padrao.
- Os dados antigos de pivo continuam preservados para evitar migracao destrutiva.
