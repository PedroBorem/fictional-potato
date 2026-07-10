# Documentacao do Projeto

Esta pasta descreve o firmware atual da Placa de Controle de Bombeamento.

## Leitura Recomendada

1. [Hardware e pinagem](functional/hardware.md)
2. [Sequencia de bombeamento](functional/pump_sequence.md)
3. [Comunicacao e padrao IDP](functional/communication_idp.md)
4. [Agendamentos por data](functional/scheduling.md)
5. [Teste serial de bancada](functional/serial_bench_test.md)
6. [Padrao de logs e cores](functional/logging.md)
7. [Persistencia e boot](functional/persistence_boot.md)
8. [Levantamento de IDPs](newproductteste_1_idp_migration.md)
9. [Limpeza do legado e pendencias](legacy_cleanup.md)
10. [Componentes](components/README.md)

## Escopo Atual

O firmware atual inicializa apenas os servicos necessarios para o controle local:

- RTC DS3231.
- NVS.
- Driver de GPIO dos relés e entradas.
- Aplicacao de atuacao com sequencia de bombeamento.
- Comunicacao serial RF UART e GPRS UART.
- Parser IDP.
- Agendamento de partida/parada por data.
- Heartbeat do ESP de conectividade pela GPRS UART.

HTTP e Wi-Fi permanecem documentados para reaproveitamento, mas nao sao inicializados nesta fase.

## Convencao de Status

| Valor | Significado |
| --- | --- |
| `0` | OFF |
| `1` | ON |
| `2` | UNKNOWN |

## Convencao de Comando

| Valor | Significado |
| --- | --- |
| `0` | NONE |
| `1` | ON / START |
| `2` | OFF / STOP |
