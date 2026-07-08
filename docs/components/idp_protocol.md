# Componente `idp_protocol`

## Situacao

Ativo no build atual.

## Funcao

Parser e serializador generico do envelope textual IDP. O componente:

- localiza um pacote entre `#` e `$`;
- identifica seu numero;
- separa campos delimitados por `-`;
- converte tipos basicos;
- monta pacotes de resposta.

| API | Uso |
| --- | --- |
| `idp_parser_get()` | Valida o envelope e extrai o IDP. |
| `idp_parser_is_payload_from_idp()` | Verifica o prefixo de um IDP. |
| `idp_parser_get_packet_data()` | Converte os campos conforme `arg_pair_t`. |
| `idp_parser_create_package()` | Serializa campos em um pacote IDP. |
| `idp_parser_get_delimiter()` | Conta delimitadores do pacote. |
| `idp_parser_remove_hashtag_cipher()` | Remove `#` e `$`. |
| `check_valid_characters()` | Valida caracteres imprimiveis. |

## Separacao de Responsabilidades

O parser nao conhece regras de pivo nem regras de bombeamento. Validacao de comandos, configuracoes, faixas de tempo e estados pertence ao modulo que executa cada IDP, atualmente `system_manager`.

Foram removidos:

- codificacao de senha de actions do pivo;
- validadores de percentimetro, sentido e irrigacao;
- validadores de configuracao de pivo e setor;
- validadores e preparacao de GPS;
- validadores de barreira fisica e virtual;
- validadores de agendamento por angulo;
- construtor antigo do IDP 28 dependente de angulo/barreira.

## Uso no Produto Atual

O `system_manager` usa o parser nos IDPs `0`, `1`, `3`, `21`, `28`, `31`, `90`, `91` e `92`. Pacotes sem handler respondem com IDP `99`.

O formato completo do IDP 3 e:

```text
#03-DEVICE_ID-OFF_RELAY_MS-IDLE_READ_SEC-STATUS_ACTIVE_LEVEL-RAMP1_SEC-STAGE1_SEC-RAMP2_SEC-STAGE2_SEC-RAMP3_SEC-STAGE3_SEC-RAMP4_SEC-STAGE4_SEC-STATUS_00_MIN$
```

Consultas usam `#03-DEVICE_ID$`. A compatibilidade com o formato basico anterior do novo produto permanece no `system_manager`, nao no parser.
