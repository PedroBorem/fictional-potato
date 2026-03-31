# Comm Protocol

## Cloud de Backup - `cloudv2-backup`

`cloudv2-backup` e um topico comum entre todos os modems.

:warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning:

CUIDADO AO PUBLICAR ALGO NESSE TOPICO, POIS IRA PARA TODOS OS CLIENTES QUE ESTIVEREM CONECTADOS NO MOMENTO!

:warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning: :warning:

Mapeamento consolidado dos IDPs encontrados nos projetos:

- `fw_PlacaDeControle_2.0`
- `fw_A7608_SIM7600`
- `fw_ICrop`
- `fw_gps_v3.0`
- `fmw_wifi-v3.0`
- `fw_outorga`

## Convencoes

- `subscricao padrao` = comando recebido pelo modem via topico do `pivot_id`, `cloudv2-backup` ou `IMEI` do modem.
- `-` = sem topico MQTT relevante no fluxo analisado, ou uso apenas local/HTTP/RF.
- `Impacto` indica onde a acao do IDP mexe diretamente: `modem`, `placa de controle`, `modem + placa de controle` ou `-`.
- `Diferencas em outros projetos` registra quando o mesmo numero de IDP muda de payload, topico ou objetivo em outro firmware.
- `Resumo do pacote` traz um formato curto ou um exemplo representativo quando isso ajuda a entender o IDP.
- `Visao rapida por projeto` lista os IDs com logica local ou variacao relevante; roteamentos simples nem sempre aparecem ali.
- `*` = IDP com observacao relevante no fim do arquivo. Em alguns casos isso indica `ainda em teste`; em outros, legado, fallback, multi-topico, timeout especial ou divergencia de implementacao.
- IDPs podem aparecer aqui mesmo sem implementacao atual quando ja existem mapeados em documentacao legada ou de refatoracao; nesses casos isso fica explicito na descricao.

## Heartbeat Local `IDP 42*`

- Objetivo: validar se `fw_A7608_SIM7600` e `fw_PlacaDeControle_2.0` continuam vivos no link serial interno.
- Intervalo atual: `30 s`.
- Timeout de resposta do primeiro `PING`: `10 s`.
- Timeout local atual: `90 s`.
- Fluxo base: `#42-PIVO_ID-PING$ -> #42-PIVO_ID-PONG$ -> #42-PIVO_ID-ACK$`.
- Publicacao MQTT: nao publica na nuvem; no modem o `IDP 42` e interceptado antes do roteamento MQTT.
- Lado da placa: o parser e o handler ficam no `system_manager`, mas o monitor de timeout fica no `system_monitoring`.
- Lado do modem: a logica periodica fica em task dedicada no modulo `board_heartbeat`.
- Falha do primeiro `PING`: se a placa nao responder dentro da janela de `10 s`, o modem executa reboot completo do ESP32 e encerra o link atual antes do `esp_restart()`.
- Falha prolongada do link na placa: se o timeout local de `90 s` expirar e o pivot estiver em `PIVOT_OFF`, a placa dispara reset interno pelo fluxo `#91$`; com `PIVOT_ON`, esse reset fica bloqueado ate o pivot desligar ou o heartbeat voltar.

## Visao Rapida por Funcao

### Actions

| IDP | Funcionalidade curta |
| --- | --- |
| `0` | Leitura do estado atual do pivot |
| `1` | Escrita das actions do pivot |
| `30` | Acao manual ou local das actions |

### Configuracoes da placa e do pivot

| IDP | Funcionalidade curta |
| --- | --- |
| `2` | Rede da placa (`gprs_id`, APN, Wi-Fi) |
| `3` | Configuracao geral do pivot |
| `4` | Rush mode / modo ECO |
| `5` | Setores |
| `21` | Timestamp |
| `22` | Barreira fisica |
| `23` | GPS / UTC / radio GPS |
| `24` | Reboot automatico |
| `26` | Barreira virtual |
| `31*` | Modo principal de comunicacao (`MQTT` ou `RF`) |
| `33` | Calibracao do sensor de pressao |
| `34*` | Configuracao do pluviometro |
| `35` | Sensibilidade de pressao / pivot molhado |
| `36` | Offset do sensor de pressao |

### Configuracoes do modem e conectividade

| IDP | Funcionalidade curta |
| --- | --- |
| `6*` | Novas configuracoes de modem / troca de `pivot_id` |
| `29` | SSID e senha do modem / pointer |
| `93` | Keep alive MQTT |
| `94` | Socket timeout MQTT |
| `95` | Tempo para APN |
| `96` | APN |
| `97` | Redes, tentativas, watchdog e ping |
| `98` | Tempo de leitura dos sensores |

### Comunicacao, status e diagnostico

| IDP | Funcionalidade curta |
| --- | --- |
| `7*` | Angulo, pressao e timestamp |
| `8` | RSSI |
| `9*` | Traceroute reservado |
| `10*` | Ruido reservado |
| `11` | Status do modem / GPRS |
| `20` | ACK interno de conexao / fluxo |
| `25` | Queda de conexao AWS |
| `40*` | Chuva da ultima hora |
| `41*` | Resumo diario de chuva |
| `42*` | Heartbeat modem <-> placa |
| `69*` | Suite de teste de hardware |

### Historico e agendamentos

| IDP | Funcionalidade curta |
| --- | --- |
| `12*` | Historico do pivot |
| `13` | Excluir agendamento |
| `14` | Liga por data e desliga por data |
| `15` | Liga por data e desliga por angulo |
| `16` | Desliga por data |
| `17*` | Desliga por angulo |
| `18` | Agendamento em execucao / espelho de exclusao |
| `27*` | Dump de agendamentos |

### Especiais e OTA

| IDP | Funcionalidade curta |
| --- | --- |
| `19*` | Leitura de pressao / pressure test |
| `28*` | Motivo do desligamento |
| `32` | Evento de rush mode / motivo do liga em alguns fluxos legados |
| `90` | Versao de firmware |
| `91*` | Reset da placa |
| `92` | Reset do modem / ACK generico |
| `99` | Erro do sistema |
| `100*` | Autorizacao e controle de OTA |
| `101*` | Validar firmware atual |
| `102*` | Forcar rollback OTA |

## Visao Rapida por Projeto

| Projeto | IDs com logica principal ou variacao relevante |
| --- | --- |
| `fw_PlacaDeControle_2.0` | `0`, `1`, `2`, `3`, `4`, `5`, `7*`, `12*`, `13`, `14`, `15`, `16`, `17*`, `18`, `21`, `22`, `23`, `24`, `26`, `28*`, `30`, `31*`, `32`, `42*`, `90`, `91*`, `92`, `34*`, `40*`, `41*`, `69*` |
| `fw_A7608_SIM7600` | `6*`, `8`, `11`, `20`, `21`, `23`, `25`, `29`, `42*`, `90`, `91*`, `92`, `93`, `94`, `95`, `96`, `97`, `99`, `100*`, `101*`, `102*`; alem do roteamento MQTT dos pacotes da placa e do fallback de `31*` |
| `fw_ICrop` | `0`, `7*`, `11`, `19*`, `23`, `25`, `29`, `33`, `35`, `36`, `90`, `92`, `93`, `94`, `95`, `96`, `97`, `98`, `99`, `100*`, `101*`, `102*` |
| `fw_gps_v3.0` | `7*`, `23`, `33` em pacotes brutos via `Serial2` |
| `fmw_wifi-v3.0` | `2`, `6*`, `8`, `11`, `12*`, `22`, `24`, `26`, `27*`, `30`, `32`, `34*`, `90`, `91*`, `99` como gateway / roteamento principal |
| `fw_outorga` | `0`, `1`, `3`, `6*`, `8`, `11`, `20`, `21`, `23`, `25`, `29`, `92`, `93`, `94`, `95`, `96`, `97`, `99`, `100*`, `101*`, `102*` |

## Tabela

| IDP | Funcionalidade | Acao | Topico | Impacto | Diferencas em outros projetos | Resumo do pacote |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | Leitura do estado atual | Monta snapshot com acoes, percentimetro, angulos e data/hora atuais. | `subscricao padrao -> cloudv2` | `-` | No `fw_ICrop` o payload vira um snapshot reduzido com DWP, percentimetro, angulos, pressao e datetime em `icrop-cloud`; no `fw_outorga` vira telemetria de pulsos, ADCs, digital e RS485 em `cloudv2-network`. | `#00-PIVO_ID-DWP-PERCENT-ANG_INI-ANG_ATUAL-DATA$` |
| 1 | Comando de acoes do pivot | Salva acoes, atua na placa, atualiza historico/barreira e depois publica o estado atual. | `subscricao padrao -> cloudv2` | `placa de controle` | No `fw_outorga` o mesmo IDP aciona diretamente um atuador GPIO e depois forca um novo ciclo de leitura com `IDP 0`. | `#01-PIVO_ID-DWP-PERCENT-AUTHOR$` |
| 2 | Configuracao de rede da placa | Le/grava `gprs_id`, APN e Wi-Fi; ao salvar, ainda emite um `IDP 6` complementar. | `subscricao padrao -> cloudv2-config` | `placa de controle` | No `fmw_wifi-v3.0` ele aparece apenas como payload roteado para `cloudv2-config`, sem nova logica local relevante. | `GET: #02-PIVO_ID$; SET/RET: #02-PIVO_ID-GPRS_ID-APN-WIFI_SSID-WIFI_PASS$` |
| 3 | Configuracao geral do pivot | Le/grava contactor, pressure e tempos; reconfigura atuacao e monitoramento. | `subscricao padrao -> cloudv2-config` | `placa de controle` | No `fw_ICrop` configura o ciclo completo do percentimetro; no `fw_outorga` passa a representar o estado persistido do atuador GPIO. | `GET: #03-PIVO_ID$; SET/RET: #03-PIVO_ID-CONTACTOR-PRESSURE-PRESS_TIME-ON_TIME-OFF_TIME-READ_TIME$` |
| 4 | Configuracao do rush mode | Le/grava janela e `enable`; reinicia o rush mode. | `subscricao padrao -> cloudv2-config` | `placa de controle` | - | `GET: #04-PIVO_ID$; SET/RET: #04-PIVO_ID-INI_HHMM-FIM_HHMM-ENABLE$` |
| 5 | Configuracao de setores | Le/grava setores e reinicia a setorizacao. | `subscricao padrao -> cloudv2-config` | `placa de controle` | - | `GET: #05-PIVO_ID$; SET/RET: #05-PIVO_ID-N_SETOR-S1_I-S1_F-S2_I-S2_F-S3_I-S3_F-S4_I-S4_F$` |
| 6* | Identificacao/config de modem | A placa responde `gprs_id` + APN; o modem tambem usa o IDP para trocar `pivot_id` e topico publico. | `subscricao padrao -> cloudv2-info`; resposta MQTT da placa cai no fallback do modem | `modem + placa de controle` | No `fmw_wifi-v3.0` troca o topico e publica `new_id` em `cloudv2-network`; no `fw_outorga` devolve `subscribed_<pivot_id>` ou `pivot_id_missing` em `cloudv2-network`. | `Placa -> modem: #06-GPRS_ID-APN$; troca de topico: pacote curto com PIVO_ID` |
| 7* | Telemetria de angulo, pressao e tempo | Atualiza RTC e angulo inicial na placa e, quando sinalizado, reenvia o payload do GPS para a nuvem. | `cloudv2-GPS` | `placa de controle` | No `fw_ICrop` publica em `icrop-gps`; no `fw_gps_v3.0` nao encontrei `IDP_7` simbolico, mas o firmware emite pacotes brutos `#07-angulo-pressao-timestamp...$`. | `Base placa: #07-ANGULO-PRESSAO-TIMESTAMP$; ICrop/GPS usam variantes mais longas` |
| 8 | RSSI do modem | Publica RSSI sob demanda e tambem no ping periodico do modem. | `subscricao padrao -> cloudv2-network`; rotina de ping -> `cloudv2-ping` | `-` | No `fmw_wifi-v3.0` um `IDP 8` recebido acaba gerando um `IDP 11` com RSSI, tecnologia e MAC; no `fw_outorga` a resposta sai em `cloudv2-network`. | `Consulta curta; retorno: #08-PIVO_ID-RSSI$` |
| 9* | Traceroute do modem | IDP ja mapeado na documentacao legada do modem para diagnostico de rota, mas sem implementacao encontrada nos firmwares analisados. | `-` | `modem` | Referencia historica: documentacao `IDP_refactor_fw/placa_v9.0-superior` o classifica como pacote barrado nos dois sentidos. No levantamento atual ele permanece reservado para uso futuro. | `Reservado; sem pacote ativo confirmado` |
| 10* | Medicao de ruido / diagnostico de sinal | IDP ja mapeado na documentacao legada do modem para leitura de ruido, mas sem implementacao encontrada nos firmwares analisados. | `-` | `modem` | Referencia historica: documentacao `IDP_refactor_fw/placa_v9.0-superior` o classifica como pacote barrado nos dois sentidos. No levantamento atual ele permanece reservado para uso futuro. | `Reservado; sem pacote ativo confirmado` |
| 11 | Status do modem | Publica snapshot de conexao no boot/reconexao e status detalhado sob consulta. | `cloud2`; `subscricao padrao -> cloudv2-info` | `-` | No `fw_ICrop` publica o status em `icrop-network`; no `fw_outorga` retorna um pacote mais completo de status do dispositivo; no `fmw_wifi-v3.0` o payload curto carrega RSSI, tecnologia e MAC. | `Consulta curta; retorno modem: #11-PIVO_ID-RSSI-TEC-TS-OPER-MODEM-IMEI-KEEPALIVE-SOCKET-APN_T-APN-REDES-TEMP-FW$` |
| 12* | Historico do pivot | Lista o historico salvo na placa. | `-` | `-` | No `fmw_wifi-v3.0` ele so e encaminhado para `cloudv2`, sem logica local equivalente ao historico da placa. | `HTTP: linhas #12-PIVO_ID-ANG_INI-ANG_FIM-TS_INI-TS_FIM-DWP-PERCENT$` |
| 13 | Exclusao de agendamento | Remove agenda por `scheduling_id`, reinicia tarefas de schedule e envia ACK. | `subscricao padrao -> cloudv2-scheduling` | `placa de controle` | - | `REQ: #13-PIVO_ID-SCHED_ID-AUTHOR$; ACK: #13-PIVO_ID-SCHED_ID$` |
| 14 | Agendamento ligar/desligar por data | Cria ou consulta agenda com `start_date`, `end_date` e acoes. | `subscricao padrao -> cloudv2-scheduling` | `placa de controle` | - | `REQ: #14-PIVO_ID-TS1-TS2-DWP-PERCENT-AUTHOR$; ACK: #14-PIVO_ID-SCHED_ID$; HTTP inclui carga completa` |
| 15 | Agendamento ligar por data e desligar por angulo | Cria ou consulta agenda com `start_date`, `end_angle` e acoes. | `subscricao padrao -> cloudv2-scheduling` | `placa de controle` | - | `REQ: #15-PIVO_ID-TS1-ANG_FIM-DWP-PERCENT-AUTHOR$; ACK: #15-PIVO_ID-SCHED_ID$; HTTP inclui carga completa` |
| 16 | Agendamento de desligamento por data | Cria ou consulta agenda so de desligamento por data. | `subscricao padrao -> cloudv2-scheduling` | `placa de controle` | - | `REQ: #16-PIVO_ID-TS_FIM-AUTHOR$; ACK: #16-PIVO_ID-SCHED_ID$; HTTP: #16-PIVO_ID-SCHED_ID-TS_FIM$` |
| 17* | Agendamento de desligamento por angulo | Cria ou consulta agenda so de desligamento por angulo. | `subscricao padrao -> cloudv2-scheduling` | `placa de controle` | - | `REQ: #17-PIVO_ID-ANG_FIM-AUTHOR$; ACK: #17-PIVO_ID-SCHED_ID$; HTTP atual no codigo usa #16-...` |
| 18 | Espelho de exclusao de agenda | Reempacota `scheduling_id` e devolve o `IDP 18` para o mesmo canal MQTT ou RF. | `subscricao padrao -> cloudv2-scheduling` | `-` | - | `#18-PIVO_ID-SCHED_ID$` |
| 19* | Leitura de pressao / snapshot reduzido | No fluxo principal do `fw_ICrop` publica DWP, pressao e data/hora; nos fluxos de pressure-test pode servir como leitura instantanea. | `fw_ICrop -> icrop-pressure`; branches antigos -> `cloudv2-pressure-test` | `-` | Deixou de ficar com `*` porque esta implementado no `master` do `fw_ICrop`; a variante `19*` da placa e do modem apareceu em `feat/pluviometer` e `feat/pluv` como leitura instantanea, ainda em teste. | `ICrop: #19-PIVO_ID-DWP-PRESSAO-DATETIME$; pressure-test legado com leitura curta` |
| 20 | Marcador do fluxo `0 -> 20` | Encerra a pendencia ou timeout usada no modem durante o fluxo de update. | `subscricao padrao` | `modem` | No `fw_outorga` o mesmo IDP esta tratado como reservado ou placeholder, sem fluxo de update equivalente. | `ACK/marcador interno; formato varia conforme o fluxo` |
| 21 | Sincronizacao de timestamp | O modem sincroniza tempo local e publica o valor atual; a placa aceita timestamp vindo da nuvem ou RF. | `subscricao padrao -> cloudv2-info` | `modem + placa de controle` | No `fw_outorga` tambem persiste o timestamp e responde via `cloudv2-network`. | `Base: #21-PIVO_ID-TIMESTAMP$` |
| 22 | Barreira fisica / retorno automatico | Le/grava angulos, retorno automatico, agua no retorno e tempo de saida da barreira. | `subscricao padrao -> cloudv2-config` | `placa de controle` | No `fmw_wifi-v3.0` ele so e encaminhado para `cloudv2-config`. | `GET: #22-PIVO_ID$; SET/RET: #22-PIVO_ID-ANG_I-ANG_F-AUTO_RETURN-WATER_RETURN-TEMPO_SAIDA$` |
| 23 | Configuracao GPS / UTC | O modem le/grava UTC; a placa consulta ou salva config GPS e encaminha para RF quando necessario. | `subscricao padrao -> cloudv2-config` | `modem + placa de controle` | No `fw_ICrop` o foco e configuracao de coordenadas e centro do pivot em `icrop-config`; no `fw_outorga` o uso ficou restrito a `UTC offset`. | `Placa: #23-PIVO_ID-SINAL_LAT-LAT-SINAL_LON-LON-TIME_PAYLOAD-OFFSET$; modem legado retorna UTC curto` |
| 24 | Reboot automatico | Le/grava `enable` e `timeout` de reboot automatico da placa. | `subscricao padrao -> cloudv2-config` | `placa de controle` | No `fmw_wifi-v3.0` ele aparece apenas como payload de configuracao encaminhado. | `GET: #24-PIVO_ID$; SET/RET: #24-PIVO_ID-ENABLE-TIMEOUT$` |
| 25 | Last will de perda de AWS | O broker publica a perda de conexao do modem quando a sessao MQTT cai. | `cloudv2-reconnection` | `-` | No `fw_ICrop` tambem existe no conjunto principal de IDs, mas nao mudou a ideia geral de reconexao. | `#25-PIVO_ID-Modem_lost_connection_to_aws$` |
| 26 | Barreira virtual / retorno automatico | Le/grava angulos e politicas da barreira virtual; reinicia monitoramento. | `subscricao padrao -> cloudv2-config` | `placa de controle` | No `fmw_wifi-v3.0` aparece apenas como encaminhamento para `cloudv2-config`; no `fw_ICrop` houve uso adicional em branch paralela de leitura no painel. | `GET: #26-PIVO_ID$; SET/RET: #26-PIVO_ID-ANG_I-ANG_F-AUTO_RETURN-WATER_RETURN$` |
| 27* | Dump de todos os schedules | Consolida agendas `14/15/16/17` em um unico payload. | `cloudv2-scheduling` | `-` | No `fmw_wifi-v3.0` o IDP tambem e roteado para `cloudv2-scheduling`; no `fw_ICrop` apareceu em branch paralela, sem semantica nova consolidada no `master`. | `#27-PIVO_ID-QTD-@14-...@15-...@16-...@17-...$` |
| 28* | Motivo de desligamento / hang up | Publica o motivo salvo da ultima parada do pivot. | `subscricao padrao -> cloudv2-shutdown` | `-` | No `fw_ICrop` houve uso em branch `feat/shutdown`, ainda sem implementacao equivalente no `master`. | `REQ: #28-PIVO_ID$; RET: #28-PIVO_ID-ORIGEM-IDP_DESLIGOU-SCHED_ID-MOTIVO-BARREIRA-POS-DATA$` |
| 29 | Wi-Fi do modem/webserver | Le/grava SSID e senha no modem. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde em `icrop-config`; no `fw_outorga` responde em `cloudv2-network`. | `GET: #29-PIVO_ID$; SET/RET: #29-PIVO_ID-SSID-SENHA$` |
| 30 | Acao manual/local do pivot | Aplica acoes locais ou de painel, atualiza historico e publica ACK ou estado atual. | `cloudv2` | `placa de controle` | No `fmw_wifi-v3.0` o IDP e apenas roteado para `cloudv2`; no `fw_ICrop` apareceu uso adicional em branch paralela. | `Entrada local: #30-DWP-PERCENT$; ACK/estado: #30-PIVO_ID-DWP-PERCENT-ANG_I-ANG_ATUAL-DATA$` |
| 31* | Modo principal de comunicacao | Le/grava se a placa usa `MQTT` ou `RF` como canal principal. | `subscricao padrao`; se a resposta sair via MQTT, cai no fallback do modem | `placa de controle` | - | `GET: #31-PIVO_ID$; SET/RET: #31-PIVO_ID-MODO$` |
| 32 | Evento de rush mode | Encapsula notificacoes como `rush_mode_deactivated`. | `cloudv2-rush` | `-` | No `fmw_wifi-v3.0` o mesmo numero de IDP e roteado para `cloudv2-pressure-test`, nao para rush mode. | `Evento curto; retorno: #32-PIVO_ID-MSG$` |
| 33 | Calibracao de pressao e tensao | Le/grava pressao minima e maxima, tensao minima e maxima e o coeficiente divisor. | `fw_ICrop -> icrop-config` | `-` | IDP novo no levantamento; encontrado implementado no `fw_ICrop`. | `GET/SET: #33-PIVO_ID-PMIN-PMAX-VMIN-VMAX-COEF$` |
| 34* | Configuracao do pluviometro | Le/grava `rain_per_pulse` e o limite de chuva para desligamento automatico; ainda em teste. | `subscricao padrao -> cloudv2-pluv` | `placa de controle` | No `fmw_wifi-v3.0` o `IDP 34` ja e roteado para `cloudv2-pressure-test`, mas a configuracao completa do pluviometro apareceu implementada localmente apenas em branches paralelas. | `GET/SET: #34-PIVO_ID-RAIN_PER_PULSE-LIMITE_CHUVA$` |
| 35 | Pressao minima para pivot molhado | Le ou grava o limite minimo de pressao usado para detectar se o pivot esta molhando. | `fw_ICrop -> icrop-config` | `-` | IDP novo no levantamento; encontrado implementado no `fw_ICrop`. | `GET/SET: #35-PIVO_ID-PRESS_WET$` |
| 36 | Offset de pressao | Le ou grava o offset com sinal e valor absoluto para correcao da leitura. | `fw_ICrop -> icrop-config` | `-` | IDP novo no levantamento; encontrado implementado no `fw_ICrop`. | `GET/SET: #36-PIVO_ID-SINAL-OFFSET$` |
| 40* | Chuva da ultima hora | Publica o acumulado da ultima hora fechada, somente quando houve chuva; ainda em teste. | `cloudv2-pluv` | `-` | Continua aparecendo como feature de pluviometro em branch paralela; nao encontrei uso equivalente consolidado nos outros firmwares analisados. | `Evento: #40-PIVO_ID-DATA-ID_HORA-MM$` |
| 41* | Resumo diario de chuva | Publica o consolidado horario do dia anterior; ainda em teste. | `cloudv2-pluv` | `-` | Continua aparecendo como feature de pluviometro em branch paralela; nao encontrei uso equivalente consolidado nos outros firmwares analisados. | `Resumo: #41-PIVO_ID-DATA[@HORA-MM...]$` |
| 42* | Heartbeat modem <-> placa | O modem envia `PING` a cada 30 s, a placa responde `PONG` e o modem fecha com `ACK`; se o primeiro `PING` nao for respondido em `10 s`, o modem executa reboot completo do ESP32. Na placa, se o timeout local expirar e o pivot estiver `PIVOT_OFF`, o monitor dispara reset interno via `#91$`; com `PIVOT_ON`, esse reset fica bloqueado. | `serial interno modem <-> placa` | `modem + placa de controle` | Nos demais projetos o `42` so aparece reservado em enums amplos, sem funcionalidade consolidada no protocolo. | `#42-PIVO_ID-PING$ -> #42-PIVO_ID-PONG$ -> #42-PIVO_ID-ACK$` |
| 69* | Suite de teste de hardware | Recebe `START` para disparar testes de hardware e aceita ou publica relatorios; ainda em teste. | `subscricao padrao -> cloudv2-test` | `placa de controle` | Ate esta passada continuou associado a branch de teste, sem uso equivalente nos demais projetos. | `Start: #69-PIVO_ID-TIMESTAMP-START$; relatorio: #69-PIVO_ID-...$` |
| 90 | Versao de firmware | Responde a versao da placa; no modem o payload ainda recebe o `TAG_VERSION` local antes da publicacao. | `subscricao padrao -> cloudv2-info` | `-` | No `fw_ICrop` publica em `icrop-update`; no `fmw_wifi-v3.0` publica em `cloudv2-network` com a versao concatenada ao payload. | `Consulta: #90-pivo_1$; retorno: #90-soil_1-v2.0.0$` |
| 91* | Reboot da placa | A placa envia ACK e executa `esp_restart()`. | `subscricao padrao`; ACK MQTT cai em `cloudv2-error` pelo fallback do modem | `placa de controle` | No `fmw_wifi-v3.0` o `IDP 91` ja e roteado explicitamente para `cloudv2-error`; no `fw_ICrop` houve uso adicional em branch paralela. | `REQ: #91-PIVO_ID$; ACK: #91-PIVO_ID$` |
| 92 | Reinicio do modem / ACK generico | O modem publica o motivo e reinicia o ESP32 apos timer; na placa existe apenas ACK local. | `subscricao padrao -> cloudv2-info` | `modem` | No `fw_ICrop` e no `fw_outorga` tambem publica a mensagem de reset em topico de rede e reinicia o sistema. | `Placa ACK: #92-PIVO_ID$; modem/rede: #92-PIVO_ID-MOTIVO$` |
| 93 | Keep alive MQTT | Le/grava keep alive no modem e em seguida dispara `IDP 92`. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde via `icrop-config`; no `fw_outorga` responde via `cloudv2-network`. | `GET: #93-PIVO_ID$; SET: #93-PIVO_ID-KEEPALIVE$` |
| 94 | Socket timeout MQTT | Le/grava socket timeout no modem e em seguida dispara `IDP 92`. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde via `icrop-config`; no `fw_outorga` responde via `cloudv2-network`. | `GET: #94-PIVO_ID$; SET: #94-PIVO_ID-SOCKET_TIMEOUT$` |
| 95 | Tempo para APN | Le/grava timeout da etapa de APN e em seguida dispara `IDP 92`. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde via `icrop-config`; no `fw_outorga` responde via `cloudv2-network`. | `GET: #95-PIVO_ID$; SET: #95-PIVO_ID-TIME_FOR_APN$` |
| 96 | APN do modem | Le/grava APN e em seguida dispara `IDP 92`. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde via `icrop-config`; no `fw_outorga` responde via `cloudv2-network`. | `GET: #96-PIVO_ID$; SET/RET: #96-PIVO_ID-APN$` |
| 97 | Parametros de rede do modem | Le/grava redes, tentativas, watchdog e ping MQTT; depois dispara `IDP 92`. | `subscricao padrao -> cloudv2-config` | `modem` | No `fw_ICrop` responde via `icrop-config` e retorna redes, tentativas e watchdog; no `fw_outorga` responde via `cloudv2-network`. | `GET: #97-PIVO_ID$; SET/RET: #97-PIVO_ID-REDES-TENTATIVAS-WATCHDOG-PING$` |
| 98 | Tempo de leitura dos sensores | Le ou grava o intervalo de leitura dos sensores. | `fw_ICrop -> icrop-config` | `-` | IDP novo no levantamento; encontrado implementado no `fw_ICrop`. | `GET: #98-PIVO_ID$; SET/RET: #98-PIVO_ID-TEMPO$` |
| 99 | Erros tecnicos | Publica erros internos do modem, OTA e sincronizacao de tempo. | `cloudv2-error` | `-` | No `fw_ICrop` publica em `icrop-error`; no `fw_outorga` e no `fmw_wifi-v3.0` manteve `cloudv2-error`. | `#99-PIVO_ID-ERRO$` |
| 100* | Controle e autorizacao de OTA | Valida pivot, IMEI e timestamp, alterna update mode e publica ACKs, erros de chunk e fim de update. | `subscricao padrao -> cloudv2-update / cloudv2-error / cloudv2-successful` | `modem` | O fluxo tambem aparece em `fw_ICrop` e `fw_outorga`, sem mudanca grande de objetivo. | `CMD: #100-PIVO_ID-ULT5_IMEI-TIMESTAMP$; ACK: #100-PIVO_ID-0/1$` |
| 101* | Validar firmware atual | Marca a app atual como valida e cancela rollback. | `subscricao padrao -> cloudv2-update` | `modem` | Tambem aparece em `fw_ICrop` e `fw_outorga`, com a mesma ideia geral de validacao. | `CMD: #101-PIVO_ID$; retorno atual: #100-PIVO_ID-update_valid_success/fail$` |
| 102* | Forcar rollback OTA | Marca a app como invalida, publica resultado e reinicia para rollback. | `subscricao padrao -> cloudv2-update` | `modem` | Tambem aparece em `fw_ICrop` e `fw_outorga`, com a mesma ideia geral de rollback. | `CMD: #102-PIVO_ID$; retorno atual: #100-PIVO_ID-rollback_triggered/fail$` |

## Observacoes

- `IDP 6*`, `IDP 31*` e o ACK MQTT do `IDP 91*` nao possuem mapeamento explicito em `topic_for_idp()` no firmware `fw_A7608_SIM7600`. Quando esses pacotes saem da placa via MQTT, o modem cai no `default`, que hoje aponta para `cloudv2-error`.
- `IDP 12*` esta implementado apenas como leitura HTTP na placa.
- `IDP 27*` e `IDP 28*` sao payloads de consolidacao ou notificacao, sem alteracao direta de estado.
- O `IDP 100*` usa mais de um topico no modem: `cloudv2-update` para ACKs e estado da OTA, `cloudv2-error` para falhas de autenticacao e erros tecnicos e `cloudv2-successful` no fechamento bem-sucedido da atualizacao.
- O `fw_gps_v3.0` nao usa `IDP_` simbolico nos arquivos pesquisados; no fluxo analisado ele emite pacotes brutos `#07-...$`, por isso apareceu apenas como variacao do `IDP 7*`.
- `IDP 9*` e `IDP 10*` foram mantidos no documento porque ja aparecem mapeados na documentacao legada do modem (`Traceroute` e `Ruido`), mas nesta varredura nao encontrei implementacao ativa deles nos seis firmwares analisados.
- O `IDP 19*` segue implementado no `master` do `fw_ICrop`; o asterisco dele agora existe porque ha observacao complementar sobre a variante legada de `pressure-test` e os usos em branch paralela.
- O `IDP 42*` foi criado como heartbeat local entre `fw_A7608_SIM7600` e `fw_PlacaDeControle_2.0`; no modem ele e interceptado antes do roteamento MQTT, entao esse handshake nao sobe para a nuvem. Sem resposta ao primeiro `PING` dentro de `10 s`, o modem faz reboot completo do ESP32. Na placa, se o timeout local ocorrer com `PIVOT_OFF`, o monitor dispara `#91$`; com `PIVOT_ON`, o reset fica pendente e bloqueado ate o desligamento do pivot ou o retorno do heartbeat.
- IDs marcados com `*` indicam que existe alguma observacao relevante nesta secao; em alguns casos isso significa `ainda em teste`, e em outros indica legado, fallback, multi-topico ou divergencia de implementacao.
- `34*`, `40*` e `41*` continuam ligados ao conjunto de branches de pluviometro, principalmente `feat/pluviometer` na placa, `feat/pluv` no modem e `feat/pluviometro` no `fw_ICrop`.
- `69*` apareceu na branch `feat/gpio-test-suite`, com publicacao no topico `cloudv2-test`.
- Ao extrair os formatos, apareceram duas inconsistencias no codigo atual: o retorno HTTP do `IDP 17*` usa envelope `#16-...` na placa e os comandos `IDP 101*` / `IDP 102*` respondem com envelopes `#100-...` no modem.
- Nesta passada, os usos extras claramente branch-only apareceram principalmente no `fw_ICrop`; em `fw_gps_v3.0`, `fmw_wifi-v3.0` e `fw_outorga` eu nao encontrei novos numeros de IDP alem dos fluxos-base analisados.
