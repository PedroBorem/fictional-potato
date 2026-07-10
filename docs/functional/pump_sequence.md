# Sequencia de Bombeamento

## Objetivo

Controlar uma partida de bombeamento de agua em 4 etapas, usando 4 relés ON mantidos energizados. A instalacao atual e one-wire e nao utiliza os relés OFF.

## Entrada de Comando

A entrada logica da regra atual e `actuation_app_set_actions()`.

Regras:

- Qualquer comando `ACTUATION_COMMAND_ON` solicita partida.
- Qualquer comando `ACTUATION_COMMAND_OFF` solicita parada segura.
- Se o mesmo payload trouxer ON e OFF, a parada tem prioridade.
- Comando invalido e ignorado.

## Estados Logicos

| Estado | Descricao |
| --- | --- |
| `STOPPED` | Bomba parada. |
| `STARTING` | Sequencia de partida em andamento. |
| `RUNNING` | Todos os canais ligados e leituras validas. |
| `STOPPING` | Parada segura em andamento. |
| `FAULT` | Falha detectada; parada segura executada. |

## Partida

1. `Canal 1 ON`
   - Liga o relay ON do canal 1.
   - Aguarda `ramp_1_delay_sec`, monitorando somente canais anteriores.
   - Ao final da rampa, valida se a leitura do canal 1 esta ON.
   - Aguarda `stage_1_delay_sec`, padrao `10 s`, monitorando o canal 1.

2. `Canal 2 ON`
   - Liga o relay ON do canal 2.
   - Aguarda `ramp_2_delay_sec`, monitorando o canal 1.
   - Ao final da rampa, valida canais 1 e 2.
   - Aguarda `stage_2_delay_sec`, padrao `30 s`, monitorando canais 1 e 2.

3. `Canal 3 ON`
   - Liga o relay ON do canal 3.
   - Aguarda `ramp_3_delay_sec`, monitorando canais 1 e 2.
   - Ao final da rampa, valida canais 1, 2 e 3.
   - Aguarda `stage_3_delay_sec`, padrao `30 s`, monitorando canais 1, 2 e 3.

4. `Canal 4 ON`
   - Liga o relay ON do canal 4.
   - Aguarda `ramp_4_delay_sec`, monitorando canais 1, 2 e 3.
   - Ao final da rampa, valida os 4 canais.
   - Aguarda `stage_4_delay_sec`, monitorando os 4 canais.
   - Entra em `RUNNING`.

Durante a rampa do motor recem-ligado, sua leitura e validada somente ao final. Isso permite que a softstarter alcance o estado de pleno funcionamento sem gerar falha prematura.

## Logs de Operacao

Ao receber comando de partida, o monitor serial registra:

- Comando START recebido e usuario de origem.
- Inicio da sequencia.
- Etapa atual: `Stage 1/4`, `Stage 2/4`, `Stage 3/4` ou `Stage 4/4`.
- Relay ON acionado em cada etapa.
- Progresso dos temporizadores a cada `CONFIG_PUMP_STAGE_LOG_INTERVAL_MS`, hoje `10000 ms`.
- Status das leituras durante o progresso do timer.
- Validacao OK de cada etapa.
- Falha de canal, quando alguma leitura esperada nao confirma ON.
- Heartbeat do timer `......` a cada `CONFIG_PUMP_STAGE_HEARTBEAT_INTERVAL_MS`, hoje `1000 ms`, somente durante a sequencia de acionamento.
- Publicacao `#29` periodica limitada por `CONFIG_PUMP_PROGRESS_PUBLISH_INTERVAL_MS`, hoje `5000 ms`, para reduzir carga na UART.

## Monitoramento em Operacao

Quando a bomba esta em `RUNNING`, a aplicacao valida as 4 leituras a cada `CONFIG_PUMP_MONITOR_INTERVAL_MS`.

Depois que a bomba entra em `RUNNING`, o log serial diminui. O acompanhamento periodico principal passa a ser o envio de `#00$` no intervalo configurado por `status_publish_time_min`.

Falha de leitura significa: canal que deveria estar ON retornou status diferente de `ACTUATION_STATUS_ON`.

## Parada Segura

A parada segura executa sempre a mesma rotina one-wire:

1. Desenergiza o relé ON do motor 4, aguarda `ramp_4_delay_sec` e valida sua leitura OFF.
2. Desenergiza o relé ON do motor 3, aguarda `ramp_3_delay_sec` e valida sua leitura OFF.
3. Desenergiza o relé ON do motor 2, aguarda `ramp_2_delay_sec` e valida sua leitura OFF.
4. Desenergiza o relé ON do motor 1, aguarda `ramp_1_delay_sec` e valida sua leitura OFF.
5. Durante cada rampa, monitora os motores que ainda deveriam permanecer ligados e publica status/progresso.

Essa rotina e usada para comando normal, agenda e falha. Numa falha durante a partida, canais ainda nao acionados sao ignorados; os canais efetivamente ligados sao desligados em ordem decrescente. Uma nova divergencia de leitura durante a parada e registrada, mas nunca interrompe a sequencia de desligamento.

Apos a parada segura, o firmware monta e salva um pacote `#28` em NVS:

- `command_off` para comando OFF, preservando o usuario recebido no IDP 1.
- `startup_fault` quando a falha ocorre antes de entrar em `RUNNING`.
- `runtime_fault` quando a falha ocorre depois da partida concluida.
- `MOTOR=1..4` quando a falha veio de leitura de um motor especifico.

O pacote `#28` tambem e publicado pelo modo principal configurado.

Durante a partida e a parada, o firmware tambem publica `#29` com o progresso da fase atual:

```text
#29-DEVICE_ID-PUMP_STATE-MOTOR-PHASE-ELAPSED_SEC-TOTAL_SEC-C1-C2-C3-C4$
```

Esse pacote foi criado para a UI local do ESP de conectividade mostrar rampa, intervalo e timer sem depender de log serial.

## Falhas

Qualquer falha durante partida ou monitoramento:

- Registra erro no log.
- Executa parada segura.
- Salva e publica o motivo via IDP 28.
- Mantem o estado logico em `FAULT`.

Uma nova solicitacao de partida pode ser aceita depois de uma falha, desde que venha um novo comando.

## Observacao Sobre o Relay ON

Na regra de bombeamento, os relés ON nao sao pulsados durante a operacao. Eles ficam energizados enquanto o canal deve permanecer ligado.

Os relés OFF permanecem desenergizados durante toda a operacao one-wire.
