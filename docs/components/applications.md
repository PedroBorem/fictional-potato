# Componente `applications`

## Situacao

Ativo parcialmente.

`components/applications/CMakeLists.txt` compila:

- `rtc_app.c`
- `data_app.c`
- `actuation_app.c`
- `comm_app.c`

## `actuation_app`

Camada principal da regra de negocio atual.

Responsabilidades:

- Validar comandos de atuacao.
- Transformar comando ON em partida sequencial de bombeamento.
- Transformar comando OFF em parada segura.
- Monitorar leituras durante partida e operacao.
- Detectar falha quando canal esperado ON nao confirma status ON.
- Persistir ultimo comando em NVS.

APIs principais:

- `actuation_app_init()`
- `actuation_app_set_config()`
- `actuation_app_set_actions()`
- `actuation_app_get_status()`
- `actuation_app_shutdown()`

## `data_app`

Camada de persistencia da aplicacao sobre `nvs_data`.

Responsabilidades atuais:

- Inicializar NVS.
- Criar valores padrao.
- Salvar e carregar `act_actions`.
- Salvar e carregar `act_config`.

Tambem preserva tipos e dados antigos de pivo para compatibilidade e reaproveitamento futuro.

## `rtc_app`

Camada de aplicacao para o RTC DS3231.

Responsabilidades:

- Inicializar descritor I2C do DS3231.
- Ajustar timestamp quando o valor recebido esta dentro da janela aceita.
- Ler timestamp e data/hora.
- Gerar strings de data/hora.

## `comm_app`

Ativo no build atual.

E a fachada entre os canais de comunicacao serial e a aplicacao:

- MQTT via `gprs`, representado por GPRS UART.
- RF via `rf_module`.

HTTP local via `http_server` permanece fora do build.

Regra: `comm_app` nao controla GPIO diretamente. Ele entrega pacotes recebidos ao callback do `system_manager`, que decide qual IDP executar.
