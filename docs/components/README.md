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

## Arquivos da Aplicacao Principal

`main/CMakeLists.txt` compila:

- `main.c`
- `system_manager.c`
- `scheduling.c`
- `system_monitoring.c`

Somente `rush_mode.c` permanece congelado fora do build para decisao posterior.

`sectorization.c` e `include/sectorization.h` foram removidos junto com a regra por setor/angulo.
