# Placa de Controle de Bombeamento

Firmware ESP-IDF para a nova placa de controle de bombeamento de agua. Este projeto usa a base do antigo firmware de placa de controle, mas a regra de negocio atual nao e mais de pivo.

O produto atual controla 4 etapas de acionamento por relés ON/OFF e monitora 4 entradas digitais de status.

## Estado Atual

- Produto: controle de bombeamento com partida sequencial.
- Hardware alvo: ESP32-S3.
- ESP-IDF: v6.0.2.
- Flash configurada: 16MB.
- Comunicacao serial: RF UART e GPRS UART ativas.
- Modo principal de comunicacao: RF por padrao.
- Agendamentos por data: IDPs 13, 14, 16 e eventos IDP 18 ativos.
- Heartbeat do ESP de conectividade: IDP 42 ativo na GPRS UART.
- HTTP/app/Wi-Fi: desabilitados nesta etapa do firmware.
- Regras de setor, barreira, GPS, pluviometro, percentimetro e agenda por angulo: removidas do firmware.
- `rush_mode.c`: preservado fora do build para decisao futura.

## Pinagem de Acionamento

| Canal | Relay ON | Relay OFF | Entrada de status |
| --- | --- | --- | --- |
| 1 | `GPIO_NUM_13` | `GPIO_NUM_12` | `GPIO_NUM_7` |
| 2 | `GPIO_NUM_11` | `GPIO_NUM_10` | `GPIO_NUM_6` |
| 3 | `GPIO_NUM_9` | `GPIO_NUM_8` | `GPIO_NUM_5` |
| 4 | `GPIO_NUM_16` | `GPIO_NUM_35` | `GPIO_NUM_4` |

Os relés da placa atual sao ativos em nivel baixo. As entradas de status tambem sao interpretadas como ON quando estao em nivel baixo.

## Funcionamento de Bombeamento

Ao receber um comando de liga, a aplicacao executa:

1. Liga o relay ON do canal 1, aguarda a rampa configurada e valida a leitura 1.
2. Aguarda o intervalo configurado do estágio 1 monitorando o canal 1.
3. Repete rampa, validacao e intervalo para os canais 2 e 3.
4. Liga o canal 4, aguarda sua rampa e intervalo configurados.
5. Valida as 4 leituras e entra em monitoramento continuo.

Se qualquer leitura esperada falhar, a rotina desliga todos os relés ON e aciona todos os relés OFF por 10 segundos.

Mais detalhes: [Sequencia de Bombeamento](docs/functional/pump_sequence.md).

## Build

Use o ESP-IDF v6.0.2:

```sh
export IDF_PATH="$HOME/.espressif/v6.0.2/esp-idf"
source "$IDF_PATH/export.sh"
idf.py set-target esp32s3
idf.py build
```

Flash e monitor:

```sh
idf.py -p /dev/cu.usbmodem1101 flash monitor
```

## Estrutura da Documentacao

- [Indice da documentacao](docs/README.md)
- [Hardware e pinagem](docs/functional/hardware.md)
- [Sequencia de bombeamento](docs/functional/pump_sequence.md)
- [Comunicacao e padrao IDP](docs/functional/communication_idp.md)
- [Agendamentos por data](docs/functional/scheduling.md)
- [Teste serial de bancada](docs/functional/serial_bench_test.md)
- [Padrao de logs e cores](docs/functional/logging.md)
- [Persistencia e boot](docs/functional/persistence_boot.md)
- [Levantamento de IDPs](docs/new_product_idp_migration.md)
- [Inventario de limpeza e pendencias](docs/legacy_cleanup.md)
- [Documentacao por componente](docs/components/README.md)

## Componentes Ativos no Build Atual

- `main`
- `applications`
- `gpio_actuator`
- `nvs_data`
- `rtc`
- `utils`
- `gprs`
- `rf_module`
- `idp_protocol`

## Componentes Preservados para a Segunda Etapa

- `wifi_app`
- `http_server`

Wi-Fi e HTTP permanecem no repositorio para reaproveitamento, mas estao excluidos do firmware atual em `CMakeLists.txt`.
