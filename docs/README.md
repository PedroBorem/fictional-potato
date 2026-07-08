# Documentacao do Projeto

Esta pasta descreve o firmware atual da Placa de Controle de Bombeamento.

## Leitura Recomendada

1. [Hardware e pinagem](functional/hardware.md)
2. [Sequencia de bombeamento](functional/pump_sequence.md)
3. [Comunicacao e padrao IDP](functional/communication_idp.md)
4. [Teste serial de bancada](functional/serial_bench_test.md)
5. [Padrao de logs e cores](functional/logging.md)
6. [Persistencia e boot](functional/persistence_boot.md)
7. [Levantamento de IDPs](new_product_idp_migration.md)
8. [Limpeza do legado e pendencias](legacy_cleanup.md)
9. [Componentes](components/README.md)

## Escopo Atual

O firmware atual inicializa apenas os servicos necessarios para o controle local:

- RTC DS3231.
- NVS.
- Driver de GPIO dos relés e entradas.
- Aplicacao de atuacao com sequencia de bombeamento.
- Comunicacao serial RF UART e GPRS UART.
- Parser IDP.

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
