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

## Pontos que Precisam de Adaptacao

O parser ainda possui validadores fortemente ligados a pivo:

- `pivot_actions`
- `pivot_config`
- agendamentos por data/angulo
- barreiras
- GPS

Para a segunda etapa, devem ser adicionados validadores especificos:

- `idp_parser_validate_actuation_actions()`
- `idp_parser_validate_actuation_config()`
- parser do `IDP 0` novo.
- parser do `IDP 1` novo.
- parser do `IDP 3` novo.

## Regra de Compatibilidade

O envelope IDP pode ser mantido. O payload deve abandonar campos de pivo e usar os tipos `actuation_actions`, `actuation_status` e `actuation_config`.
