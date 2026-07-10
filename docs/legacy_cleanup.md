# Limpeza do Legado e Pendencias

## Removido

- Regras e estruturas de pivo, setor, angulo, GPS e barreiras.
- Agendamentos por angulo.
- IDP 24 e configuracao de reboot automatico.
- IDP 92 e expectativa de reset fisico de modem.
- Chaves NVS `reboot_config` e `s_start_state`.
- Dados NVS antigos de pivo, apagados de forma idempotente no boot.

IDPs removidos ou ainda sem funcao respondem `#99-DEVICE_ID-unsupported_idp$`.

## Operacional

| Frente | Situacao |
| --- | --- |
| Bombeamento | Partida sequencial, rampas, validacao, monitoramento e parada segura. |
| UART RF | Recebe e envia IDPs via modulo LoraMesh; canal principal padrao. |
| UART GPRS | Recebe e envia IDPs via ESP de conectividade. |
| IDP 42 | Heartbeat com `PING`/`PONG`, intervalo de 30 s e timeout de 90 s. |
| IDPs 13, 14, 16 e 18 | Exclusao, partida/parada por data, apenas parada por data e eventos. |
| NVS de agenda | `s_date` e `s_off_date` persistem as agendas novas. |

O timeout do IDP 42 apenas gera warning. Nao existe controle eletrico de reset ou alimentacao do ESP de conectividade nesta placa.

## Mantido Sem Adaptacao

- `wifi_app` e `http_server`, fora do build.
- `main/rush_mode.c`, fora do build.
- NVS `rush_config`, `rush_state` e `history`.
- NVS `net_config`, preservada por compatibilidade.

## Pendente de Definicao ou Adaptacao

| Frente | Situacao |
| --- | --- |
| IDPs 2 e 6 | Definir somente se a placa precisar configurar o ESP de conectividade pela UART. |
| Contrato do ESP de conectividade | Definir no outro firmware broker, topicos, autenticacao, QoS, ACK e retry. |
| IDP 12 | Implementado como historico de operacoes do bombeamento salvo em NVS. |
| IDP 30 | Definir comando/evento manual local. |
| Rush mode | Discutir se existe regra equivalente no produto de bombeamento. |
| Wi-Fi/HTTP local | Decidir se voltam ao produto antes de adaptar os componentes preservados. |

## Ordem Recomendada

1. Validar em bancada heartbeat e agendas com atrasos curtos.
2. Validar persistencia e comportamento de agendas depois de reboot.
3. Fechar no firmware de conectividade o encaminhamento transparente dos IDPs e o contrato MQTT.
4. Decidir o contrato dos IDPs 2 e 6 entre as duas placas.
5. Definir comando local IDP 30.
6. Discutir rush mode e remover o arquivo se a regra nao fizer sentido.
