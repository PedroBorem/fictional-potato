# Limpeza do Legado e Pendencias

## Removido

- Regra e arquivos de setorizacao por angulo.
- Estruturas compartilhadas de configuracao de pivo, setor, GPS e barreiras.
- Estruturas de agendamento por angulo.
- Validadores IDP de actions do pivo, percentimetro, sentido, setor, GPS, barreiras e agenda por angulo.
- Construtor antigo do IDP 28 baseado em angulo e barreira.
- Globais `global_angle` e `counter_reading_panel_off`.
- Criacao, leitura e escrita das chaves NVS descartadas.
- Dados antigos dessas chaves, apagados de forma idempotente no boot.

IDPs sem funcao no novo produto continuam sem handler e respondem `#99-DEVICE_ID-unsupported_idp$`.

## Mantido Sem Adaptacao

- `wifi_app` e `http_server`, fora do build.
- `main/rush_mode.c`, fora do build.
- `main/scheduling.c`, fora do build; apenas agenda por horario podera ser reaproveitada.
- `main/system_monitoring.c`, fora do build.
- NVS `rush_config`, `rush_state` e `history`.
- Configuracao reservada de rede, reboot e agenda por data.
- IDP 24, reservado para reboot automatico geral.

## Pendente de Definicao ou Adaptacao

| Frente | Situacao |
| --- | --- |
| GPRS/MQTT real | Definir modem, protocolo, broker, topicos, autenticacao, ACK e retry. |
| IDPs 2 e 6 | Redesenhar configuracao de rede/modem. |
| IDP 12 | Criar historico de comandos, partidas, paradas e falhas. |
| IDP 24 | Adaptar reboot automatico geral ao novo produto. |
| IDP 30 | Definir comando/evento manual local. |
| IDP 42 | Implementar heartbeat entre modem e placa. |
| IDP 92 | Executar reset fisico/real do modem. |
| Agenda por horario | Redesenhar actions e persistencia sem campos de pivo. |

## Ordem Recomendada

1. Validar em bancada partida, rampas, falhas, parada e IDPs operacionais.
2. Definir e implementar heartbeat e reset real do modem.
3. Fechar o contrato GPRS/MQTT e adaptar IDPs 2 e 6.
4. Adaptar agenda por horario e IDP 24.
5. Implementar historico IDP 12 e comando local IDP 30.
6. Reavaliar `rush_mode.c`, `scheduling.c` e `system_monitoring.c`; adaptar apenas as partes aprovadas e apagar o restante.
