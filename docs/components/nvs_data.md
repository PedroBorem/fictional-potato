# Componente `nvs_data`

## Situacao

Ativo no build atual.

## Funcao

Wrapper direto da API NVS do ESP-IDF.

## Responsabilidades

- Inicializar `nvs_flash`.
- Apagar e reinicializar NVS se houver erro de paginas sem espaco ou versao nova.
- Salvar blobs.
- Ler tamanho de blobs.
- Carregar blobs.

## APIs

| API | Funcao |
| --- | --- |
| `nvs_data_init()` | Inicializa NVS flash. |
| `nvs_data_set()` | Salva blob usando o proprio label como namespace e chave. |
| `nvs_data_get_size()` | Retorna tamanho do blob salvo. |
| `nvs_data_get_blob()` | Carrega blob para buffer de saida. |

## Uso no Produto Atual

O acesso direto a esse componente deve continuar concentrado em `data_app`.

Chaves novas relevantes:

- `act_actions`
- `act_config`
