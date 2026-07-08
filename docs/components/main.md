# Componente `main`

## Situacao

Ativo no build atual.

`main/CMakeLists.txt` compila:

- `main.c`
- `system_manager.c`

## Funcao

E o ponto de entrada do firmware. Ele inicializa o sistema e mantem a aplicacao viva.

## `main.c`

Chama `system_manager_init()` dentro de `app_main()` e depois permanece em loop com delay.

## `system_manager.c`

Inicializa somente os servicos necessarios ao novo produto:

1. RTC.
2. NVS/data app.
3. Configuracao de atuacao.
4. Modo principal de comunicacao.
5. Aplicacao de atuacao.
6. Comunicacao serial RF/GPRS.
7. Parser IDP.
8. Publicacao do ultimo motivo de desligamento via IDP 28.
9. Leitura inicial de status.

O `system_manager` tambem monta, salva e responde o IDP 28 usando a chave NVS `reason_hangup`.

Nao inicializa:

- Comunicacao HTTP/app.
- Wi-Fi.
- Regras de pivo.

## Estado Global

- `global_pressure`: preservado, ainda sem uso na regra nova.
- `comm_main_mode`: canal principal de eventos espontaneos.
- `system_id`: identificador do equipamento nos pacotes IDP.

Os globais de angulo e contador manual do pivo foram removidos.

## Arquivos Legados em `main/`

| Arquivo | Status |
| --- | --- |
| `rush_mode.c` | Legado de pivo, fora do build. |
| `scheduling.c` | Legado de agendas de pivo, fora do build. |
| `system_monitoring.c` | Legado de monitoramento/heartbeat, fora do build. |

`sectorization.c` e seu header foram removidos.
