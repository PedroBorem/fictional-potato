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
- Registrar o ultimo evento de desligamento para publicacao do IDP 28.

APIs principais:

- `actuation_app_init()`
- `actuation_app_set_config()`
- `actuation_app_set_actions()`
- `actuation_app_get_status()`
- `actuation_app_get_shutdown_info()`
- `actuation_app_shutdown()`

## `data_app`

Camada de persistencia da aplicacao sobre `nvs_data`.

Responsabilidades atuais:

- Inicializar NVS.
- Criar valores padrao.
- Salvar e carregar `act_actions`.
- Salvar e carregar `act_config`.
- Salvar e carregar `reason_hangup`, usado pelo IDP 28.
- Salvar e carregar `s_date` e `s_off_date`, usados pelos IDPs 14 e 16.
- Remover de forma idempotente as chaves NVS descartadas do produto antigo.

Continuam reservados, sem uso no fluxo atual: configuracao de rede, rush mode e historico. As antigas chaves `reboot_config` e `s_start_state` sao removidas no boot.

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

- ESP de conectividade via `gprs`, representado internamente como `COMM_MQTT`.
- RF via `rf_module`.

HTTP local via `http_server` permanece fora do build.

Regra: `comm_app` nao controla GPIO diretamente. Ele entrega pacotes recebidos ao callback do `system_manager`, que decide qual IDP executar.

O broker MQTT e seus topicos sao responsabilidade do outro ESP32. Nesta placa, `gprs` e somente um transporte UART de pacotes IDP.
