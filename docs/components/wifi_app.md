# Componente `wifi_app`

## Situacao

Inativo no build atual.

Esta listado em `EXCLUDE_COMPONENTS` no `CMakeLists.txt` raiz.

## Funcao Preservada

Subir um ponto de acesso Wi-Fi local para configuracao/manutencao.

## Configuracao Historica

| Item | Valor |
| --- | --- |
| Modo | SoftAP |
| IP | `192.168.0.1` |
| Mascara | `255.255.255.0` |
| Canal | `7` |
| Maximo de estacoes | `5` |

## Fluxo Esperado Quando Reativado

1. `comm_app` configura SSID/senha.
2. `wifi_app` sobe/recarrega SoftAP.
3. `http_server` atende comandos locais.

## Observacao

Wi-Fi local deve ser tratado como canal de manutencao. A regra de bombeamento deve continuar encapsulada em `actuation_app`.
