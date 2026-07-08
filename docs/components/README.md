# Documentacao por Componente

## Componentes Ativos

| Componente | Documento | Situacao |
| --- | --- | --- |
| `main` | [main.md](main.md) | Ativo |
| `applications` | [applications.md](applications.md) | Ativo parcialmente |
| `gpio_actuator` | [gpio_actuator.md](gpio_actuator.md) | Ativo |
| `nvs_data` | [nvs_data.md](nvs_data.md) | Ativo |
| `rtc` | [rtc.md](rtc.md) | Ativo |
| `utils` | [utils.md](utils.md) | Ativo |
| `gprs` | [gprs.md](gprs.md) | Ativo |
| `rf_module` | [rf_module.md](rf_module.md) | Ativo |
| `idp_protocol` | [idp_protocol.md](idp_protocol.md) | Ativo |

## Componentes Inativos no Build Atual

| Componente | Documento | Motivo |
| --- | --- | --- |
| `wifi_app` | [wifi_app.md](wifi_app.md) | HTTP/app local desabilitado nesta fase. |
| `http_server` | [http_server.md](http_server.md) | HTTP/app local desabilitado nesta fase. |

## Codigo Legado Fora do Build

Arquivos congelados permanecem em `main/`, mas `main/CMakeLists.txt` compila apenas `main.c` e `system_manager.c`.

Arquivos legados:

- `rush_mode.c`
- `scheduling.c`
- `system_monitoring.c`

`sectorization.c` e `include/sectorization.h` foram removidos junto com a regra por setor/angulo.
