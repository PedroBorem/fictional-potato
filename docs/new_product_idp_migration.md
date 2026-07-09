# Novo Produto - Levantamento de IDPs

## Escopo

O firmware controla quatro motores de bombeamento e nao inicializa nenhuma regra de pivo. RF UART e GPRS UART transportam pacotes IDP; HTTP e Wi-Fi continuam fora do build.

Na GPRS UART, o equipamento remoto e outro ESP32. Ele cuida da conectividade, broker e topicos MQTT. Esta placa apenas recebe e envia IDPs brutos.

## Produto Atual

| Canal | Relay ON | Relay OFF | Leitura de status |
| --- | --- | --- | --- |
| 1 | `GPIO_NUM_13` | `GPIO_NUM_12` | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_11` | `GPIO_NUM_10` | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_9` | `GPIO_NUM_8` | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_16` | `GPIO_NUM_35` | `GPIO_NUM_4` |

Cada motor possui rampa de estabilizacao e intervalo posterior configuraveis pelo IDP 3. Uma falha durante partida ou operacao desliga todos os relés ON e pulsa os relés OFF.

## IDPs Operacionais

| IDP | Situacao | Uso no produto |
| --- | --- | --- |
| `0` | Adaptado | Snapshot do bombeamento e das quatro leituras. |
| `1` | Adaptado | Partida sequencial ou parada segura. |
| `3` | Adaptado | Rampas, intervalos, leitura, nivel ativo e telemetria. |
| `13` | Adaptado | Exclui agenda pelo identificador. |
| `14` | Adaptado | Agenda partida e parada por data. |
| `16` | Adaptado | Agenda somente parada por data. |
| `18` | Adaptado | Publica evento de execucao da agenda. |
| `21` | Reaproveitado | Sincroniza o RTC. |
| `28` | Adaptado | Retem e publica motivo de desligamento. |
| `31` | Reaproveitado | Seleciona RF ou GPRS UART para eventos espontaneos. |
| `42` | Adaptado | Heartbeat com o ESP de conectividade pela GPRS UART. |
| `90` | Reaproveitado | Retorna versao do firmware. |
| `91` | Reaproveitado | Reinicia este ESP32-S3. |

## IDPs Pendentes

| IDP | Decisao pendente |
| --- | --- |
| `2` e `6` | Definir se configurarao o ESP de conectividade e qual sera o contrato UART. |
| `12` | Criar historico do novo produto. |
| `30` | Definir comando ou evento manual/local. |

O protocolo MQTT, incluindo topicos, credenciais, QoS, ACK e retry, sera definido no firmware do ESP de conectividade.

## IDPs Removidos ou Inutilizados

| IDP | Motivo |
| --- | --- |
| `4` e `32` | Rush mode antigo nao foi adaptado; decisao futura. |
| `5`, `7`, `15`, `17`, `22`, `23`, `26` e `27` | Dependiam de angulo, setor, GPS ou barreiras. |
| `24` | Reboot automatico foi removido por decisao de produto. |
| `34`, `40` e `41` | Pluviometro nao faz parte do produto. |
| `69` | Suite antiga de teste nao faz parte da regra principal. |
| `92` | Nao existe reset fisico do ESP de conectividade nesta placa. |

## Contratos Principais

```text
#00-DEVICE_ID-PUMP_STATE-C1-C2-C3-C4-LAST_FAULT-TIMESTAMP$
#01-DEVICE_ID-CMD_C1-CMD_C2-CMD_C3-CMD_C4-USER$
#03-DEVICE_ID-OFF_RELAY_MS-IDLE_READ_SEC-STATUS_ACTIVE_LEVEL-RAMP1_SEC-STAGE1_SEC-RAMP2_SEC-STAGE2_SEC-RAMP3_SEC-STAGE3_SEC-RAMP4_SEC-STAGE4_SEC-STATUS_00_MIN$
#13-DEVICE_ID-SCHEDULE_ID-USER$
#14-DEVICE_ID-START_TIME-END_TIME-USER$
#16-DEVICE_ID-END_TIME-USER$
#18-DEVICE_ID-SOURCE_IDP-SCHEDULE_ID-EVENT-USER-TIMESTAMP$
#28-DEVICE_ID-REASON-ORIGIN-USER-PHASE-MOTOR-RESET_REASON-TIMESTAMP$
#42-DEVICE_ID-PING$
#42-DEVICE_ID-PONG$
```

Detalhes: [Protocolo de Comunicacao](../comm_protocol.md) e [Agendamentos por Data](functional/scheduling.md).

## Situacao no Codigo

- `main/CMakeLists.txt` compila `main.c`, `system_manager.c`, `scheduling.c` e `system_monitoring.c`.
- `system_manager.c` integra RTC, NVS, atuacao, duas UARTs, IDPs, agenda e heartbeat.
- `components/applications/CMakeLists.txt` compila `rtc_app.c`, `data_app.c`, `actuation_app.c` e `comm_app.c`.
- `gprs`, `rf_module` e `idp_protocol` entram no build atual.
- `wifi_app` e `http_server` continuam excluidos.
- RF e o modo principal padrao.
- Agendas ficam em `s_date` e `s_off_date`; o ultimo desligamento fica em `reason_hangup`.
