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
- HTTP/app/Wi-Fi: desabilitados nesta etapa do firmware.
- Regra de pivo, rush mode, setor, barreira, GPS e agenda por angulo: legado preservado no repositorio, mas fora do build atual.

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

1. Liga o relay ON do canal 1 e aguarda 10 segundos.
2. Valida a leitura do canal 1.
3. Liga o relay ON do canal 2 e aguarda 30 segundos monitorando os canais ja ativos.
4. Valida as leituras dos canais 1 e 2.
5. Liga o relay ON do canal 3 e aguarda 30 segundos monitorando os canais ja ativos.
6. Valida as leituras dos canais 1, 2 e 3.
7. Liga o relay ON do canal 4.
8. Valida as 4 leituras e entra em monitoramento continuo.

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
- [Teste serial de bancada](docs/functional/serial_bench_test.md)
- [Persistencia e boot](docs/functional/persistence_boot.md)
- [Levantamento de IDPs](docs/new_product_idp_migration.md)
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
