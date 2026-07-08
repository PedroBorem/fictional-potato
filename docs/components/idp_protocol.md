# Componente `idp_protocol`

## Situacao

Ativo no build atual.

## Funcao Preservada

Parser e gerador de pacotes IDP. O firmware atual usa o parser para identificar e extrair os IDPs de bombeamento no `system_manager`.

## Funcoes Reaproveitaveis

| API | Uso |
| --- | --- |
| `idp_parser_get()` | Extrai IDP de um payload `#...$`. |
| `idp_parser_is_payload_from_idp()` | Verifica rapidamente se o payload pertence a um IDP. |
| `idp_parser_remove_hashtag_cipher()` | Remove `#` e `$`. |
| `idp_parser_create_package()` | Monta pacote com argumentos. |
| `idp_parser_get_packet_data()` | Extrai campos do pacote. |

## Implementacao do Novo Produto

O `system_manager` usa o parser generico para os IDPs `0`, `1`, `3`, `21`, `31`, `90`, `91` e `92`.

O IDP 3 implementado usa:

```text
#03-DEVICE_ID-OFF_RELAY_MS-IDLE_READ_SEC-STATUS_ACTIVE_LEVEL-RAMP1_SEC-STAGE1_SEC-RAMP2_SEC-STAGE2_SEC-RAMP3_SEC-STAGE3_SEC-RAMP4_SEC-STAGE4_SEC-STATUS_00_MIN$
```

O parser aceita consulta com `#03-DEVICE_ID$`, o formato completo atual e o formato basico legado com `OFF_RELAY_MS`, `IDLE_READ_SEC` e `STATUS_ACTIVE_LEVEL`.

## Pontos Ainda Legados

O parser ainda possui validadores fortemente ligados a pivo:

- `pivot_actions`
- `pivot_config`
- agendamentos por data/angulo
- barreiras
- GPS

Uma evolucao futura pode mover para o componente validadores especificos que hoje estao no `system_manager`:

- `idp_parser_validate_actuation_actions()`
- `idp_parser_validate_actuation_config()`

## Regra de Compatibilidade

O envelope IDP pode ser mantido. O payload deve abandonar campos de pivo e usar os tipos `actuation_actions`, `actuation_status` e `actuation_config`.
