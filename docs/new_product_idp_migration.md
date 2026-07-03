# Novo Produto - Levantamento de IDPs

## Escopo desta etapa

O firmware deixa de inicializar a regra de negocio de pivo. Nesta fase tambem nao inicializa `comm_app`, Wi-Fi, GPRS, RF nem HTTP server. Os IDPs abaixo ficam mapeados para a segunda etapa de comunicacao.

O produto atual passa a ter:

| Canal | Relay ON | Relay OFF | Leitura de status |
| --- | --- | --- | --- |
| 1 | `GPIO_NUM_13` | `GPIO_NUM_12` | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_11` | `GPIO_NUM_10` | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_9` | `GPIO_NUM_8` | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_16` | `GPIO_NUM_35` | `GPIO_NUM_4` |

## IDPs que podem ser aproveitados

| IDP | Uso atual | Como aproveitar |
| --- | --- | --- |
| `2` | Configuracao de rede da placa | Manter para a fase de comunicacao, caso GPRS/Wi-Fi continuem no produto. |
| `6` | Configuracao/identificacao do modem | Manter se o modem continuar usando o mesmo fluxo de identificacao. |
| `21` | Sincronizacao de timestamp | Aproveitar sem depender de pivo. |
| `24` | Reboot automatico | Aproveitar como configuracao geral da placa. |
| `31` | Modo principal de comunicacao | Aproveitar se a nova fase ainda alternar entre MQTT/RF. |
| `42` | Heartbeat modem/placa | Aproveitar se o modem continuar conectado a placa por UART. |
| `90` | Versao de firmware | Aproveitar sem mudanca conceitual. |
| `91` | Reset da placa | Aproveitar sem mudanca conceitual. |
| `92` | Reset do modem / ACK generico | Aproveitar se o modem continuar no hardware final. |

## IDPs que devem ser adaptados

| IDP | Uso antigo | Adaptacao recomendada |
| --- | --- | --- |
| `0` | Leitura de estado do pivo | Virar leitura dos quatro status ON/OFF. |
| `1` | Escrita de actions do pivo | Virar comando dos quatro canais, com `ON`, `OFF` ou `NONE` por canal. |
| `3` | Configuracao geral do pivo | Virar configuracao de atuacao: tempo de pulso dos relays, intervalo de leitura e nivel ativo das entradas. |
| `12` | Historico do pivo | Adaptar apenas se o novo produto precisar de historico de mudanca de status/comando. |
| `13` a `18` | Agendamentos de pivo por data/angulo | Adaptar somente se houver agendamento por canal; remover qualquer conceito de angulo. |
| `27` | Dump de agendamentos | Adaptar junto com novos agendamentos por canal, se existirem. |
| `28` | Motivo de desligamento do pivo | Adaptar para eventos/falhas dos acionamentos, se o produto precisar de diagnostico. |
| `30` | Acao manual/local do pivo | Adaptar para mudanca local detectada nas entradas de status. |

## IDPs inutilizados para este produto

| IDP | Motivo |
| --- | --- |
| `4` | Rush mode e janela ECO sao regra de pivo. |
| `5` | Setorizacao por angulo nao existe no novo produto. |
| `7` | Angulo/pressao/GPS do pivo nao fazem parte dos 4 acionamentos ON/OFF. |
| `22` | Barreira fisica do pivo nao existe no novo produto. |
| `23` | GPS/centro do pivo nao faz parte do produto atual. |
| `26` | Barreira virtual do pivo nao existe no novo produto. |
| `32` | Evento de rush mode deixa de existir. |

## Situacao no codigo

- `main/CMakeLists.txt` compila apenas `main.c` e `system_manager.c`.
- `system_manager.c` inicializa RTC, NVS e atuacao local.
- `components/applications/CMakeLists.txt` nao compila `comm_app.c`, portanto HTTP/app fica fora da inicializacao nesta etapa.
- `rush_mode.c`, `system_monitoring.c`, `sectorization.c` e `scheduling.c` permanecem no repositorio como legado para consulta, mas nao entram no firmware atual.
- Os dados NVS novos sao `act_actions` e `act_config`; os dados antigos de pivo continuam intactos para evitar migracao destrutiva agora.
