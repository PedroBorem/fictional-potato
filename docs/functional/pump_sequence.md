# Sequencia de Bombeamento

## Objetivo

Controlar uma partida de bombeamento de agua em 4 etapas, usando 4 relés ON mantidos energizados e 4 relés OFF usados na parada segura.

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
   - Aguarda `CONFIG_PUMP_STAGE_1_DELAY_MS`, hoje `10000 ms`.
   - Valida se leitura do canal 1 esta ON.

2. `Canal 2 ON`
   - Liga o relay ON do canal 2.
   - Aguarda `CONFIG_PUMP_STAGE_2_DELAY_MS`, hoje `30000 ms`.
   - Durante a espera, monitora canal 1.
   - Ao final, valida canais 1 e 2.

3. `Canal 3 ON`
   - Liga o relay ON do canal 3.
   - Aguarda `CONFIG_PUMP_STAGE_3_DELAY_MS`, hoje `30000 ms`.
   - Durante a espera, monitora canais 1 e 2.
   - Ao final, valida canais 1, 2 e 3.

4. `Canal 4 ON`
   - Liga o relay ON do canal 4.
   - Aguarda um ciclo de monitoramento, hoje `CONFIG_PUMP_MONITOR_INTERVAL_MS = 500 ms`.
   - Valida os 4 canais.
   - Entra em `RUNNING`.

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

## Monitoramento em Operacao

Quando a bomba esta em `RUNNING`, a aplicacao valida as 4 leituras a cada `CONFIG_PUMP_MONITOR_INTERVAL_MS`.

O monitor serial tambem registra um status periodico a cada `CONFIG_PUMP_STAGE_LOG_INTERVAL_MS`, hoje `10000 ms`.

Falha de leitura significa: canal que deveria estar ON retornou status diferente de `ACTUATION_STATUS_ON`.

## Parada Segura

A parada segura executa sempre a mesma rotina:

1. Desenergiza todos os relés ON.
2. Energiza todos os relés OFF.
3. Aguarda `CONFIG_PUMP_STOP_RELAY_TIME_MS`, hoje `10000 ms`.
4. Desenergiza todos os relés OFF.
5. Persiste a ultima acao como OFF.

Essa rotina e usada tanto para comando normal de parada quanto para falha.

## Falhas

Qualquer falha durante partida ou monitoramento:

- Registra erro no log.
- Executa parada segura.
- Mantem o estado logico em `FAULT`.

Uma nova solicitacao de partida pode ser aceita depois de uma falha, desde que venha um novo comando.

## Observacao Sobre o Relay ON

Na regra de bombeamento, os relés ON nao sao pulsados durante a operacao. Eles ficam energizados enquanto o canal deve permanecer ligado.

Os relés OFF sao acionados por tempo limitado durante a parada.
