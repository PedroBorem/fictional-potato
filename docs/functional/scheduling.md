# Agendamentos por Data

## Escopo

O firmware executa dois tipos de agenda persistidos em NVS:

- IDP 14: liga na data inicial e desliga na data final.
- IDP 16: apenas desliga na data informada.

O IDP 13 exclui uma agenda e o IDP 18 publica cada execucao. Agendas por angulo nao existem no novo produto.

## Representacao de Data

Os campos de data aceitam:

- Unix timestamp absoluto quando o valor for maior ou igual a `1000000000`.
- Atraso em segundos a partir do RTC atual para valores positivos menores.

Exemplo para teste imediato: `60` significa daqui a 60 segundos. Em producao, prefira Unix timestamp sincronizado pelo IDP 21.

## IDP 14 - Liga e Desliga

Criacao:

```text
#14-DEVICE_ID-START_TIME-END_TIME-USER$
```

Resposta:

```text
#14-DEVICE_ID-SCHEDULE_ID-ACCEPTED$
```

Consulta:

```text
#14-DEVICE_ID$
```

Cada agenda e retornada separadamente:

```text
#14-DEVICE_ID-SCHEDULE_ID-START_UNIX-END_UNIX-STARTED-USER$
```

Sem agendas:

```text
#14-DEVICE_ID-NONE$
```

Ao atingir `START_TIME`, o agendador solicita a mesma partida sequencial usada pelo IDP 1. Ao atingir `END_TIME`, solicita a parada segura completa.

## IDP 16 - Apenas Desliga

Criacao:

```text
#16-DEVICE_ID-END_TIME-USER$
```

Resposta:

```text
#16-DEVICE_ID-SCHEDULE_ID-ACCEPTED$
```

Consulta:

```text
#16-DEVICE_ID$
```

Cada agenda e retornada separadamente:

```text
#16-DEVICE_ID-SCHEDULE_ID-END_UNIX-USER$
```

Sem agendas:

```text
#16-DEVICE_ID-NONE$
```

## IDP 13 - Excluir

```text
#13-DEVICE_ID-SCHEDULE_ID-USER$
```

O campo `USER` e opcional. Resposta:

```text
#13-DEVICE_ID-SCHEDULE_ID-DELETED$
```

## IDP 18 - Evento

Eventos espontaneos saem pelo modo principal configurado no IDP 31:

```text
#18-DEVICE_ID-SOURCE_IDP-SCHEDULE_ID-EVENT-USER-TIMESTAMP$
```

`SOURCE_IDP` vale `14` ou `16`. `EVENT` pode ser:

- `STARTED`: comando de partida emitido.
- `STOPPED`: comando de parada emitido.
- `EXPIRED`: partida perdida fora da tolerancia.

## Regras e Boot

- Sao aceitas ate 10 agendas IDP 14 e 10 agendas IDP 16.
- Janelas IDP 14 sobrepostas sao rejeitadas com `schedule_conflict`.
- Uma partida pendente atrasada em ate 30 minutos ainda e executada.
- Uma partida atrasada mais de 30 minutos gera `EXPIRED`.
- Uma agenda IDP 14 marcada como iniciada antes de um reboot nao religa os motores no boot; apenas preserva a parada na data final.
- Agendas futuras ainda pendentes permanecem na NVS e podem partir depois do boot.
- O RTC deve estar correto para agendas com Unix timestamp.

## Teste Rapido

Agenda para ligar em 10 segundos e desligar em 90 segundos:

```text
#14-newproduct_1-10-90-bancada$
```

Agenda apenas para desligar em 30 segundos:

```text
#16-newproduct_1-30-bancada$
```

Consulte as agendas e use o `SCHEDULE_ID` retornado para testar o IDP 13.
