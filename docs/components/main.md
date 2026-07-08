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
8. Leitura inicial de status.

Nao inicializa:

- Comunicacao HTTP/app.
- Wi-Fi.
- Regras de pivo.

## Globais Legadas

Algumas variaveis globais permanecem para compatibilidade com headers e codigo legado:

- `global_pressure`
- `global_angle`
- `comm_main_mode`
- `system_id`
- `counter_reading_panel_off`

Elas nao definem a regra principal do bombeamento nesta fase.

## Arquivos Legados em `main/`

| Arquivo | Status |
| --- | --- |
| `rush_mode.c` | Legado de pivo, fora do build. |
| `scheduling.c` | Legado de agendas de pivo, fora do build. |
| `sectorization.c` | Legado de setores por angulo, fora do build. |
| `system_monitoring.c` | Legado de monitoramento/heartbeat, fora do build. |
