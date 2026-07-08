# Componente `rtc`

## Situacao

Ativo no build atual.

## Funcao

Driver do RTC DS3231 por I2C.

## Arquivos

- `rtc_i2c_dev.c`
- `rtc_ds3231.c`

## Papel no Firmware

O componente fornece as funcoes de baixo nivel usadas por `rtc_app`:

- Inicializacao do dispositivo I2C.
- Leitura de data/hora.
- Escrita de data/hora.

## Pinagem Usada pela Aplicacao

A pinagem e definida em `rtc_app.c`:

| Sinal | GPIO |
| --- | --- |
| SDA | `GPIO_NUM_36` |
| SCL | `GPIO_NUM_37` |

## Observacao

A regra de bombeamento atual nao depende de horario para partir ou parar, mas o RTC permanece ativo para timestamps, logs e futura comunicacao IDP.
