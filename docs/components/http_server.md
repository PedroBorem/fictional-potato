# Componente `http_server`

## Situacao

Inativo no build atual.

Esta listado em `EXCLUDE_COMPONENTS` no `CMakeLists.txt` raiz.

## Funcao Preservada

Servidor HTTP local para receber comandos, configuracoes e consultas.

## Comportamento Historico

- Registra handlers wildcard para `GET`, `POST` e `DELETE`.
- Recebe payload HTTP.
- Entrega eventos para callback da aplicacao.
- Envia resposta usando `http_server_send_resp()`.

## Fluxo Esperado Quando Reativado

HTTP deve aceitar comandos no padrao IDP novo:

```text
#01-newproductteste_1-1-0-0-0-app$
```

E deve responder com snapshot ou ACK:

```text
#00-newproductteste_1-STARTING-1-0-0-0-0-TS$
```

## Regra Importante

O servidor HTTP nao deve acionar GPIO diretamente. Ele deve passar pelo mesmo caminho de comando usado por MQTT/RF.
