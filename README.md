# Cluster Researches

## Сборка ноды кластера

Ветка `eventloop`

Сборка необходимых исполняемых файлов:
```shell script
cmake . && make
```

Необходимо настроить конфиг файлы `processing-daemon-config.json`
```json
{
  "observer-ip" : "<insert here observer IP>",
  "observer-port" : 0,

  "entry-point" : [
    {
      "host-info" : "semen",
      "address" : {
        "ip" : "<insert here host IP>",
        "port" : 0
      },
      ...
    }
  ]
}
```
и `processing-observer-config.json`
```json
{
  "observer-ip" : "<insert here observer IP>",
  "observer-port" : 0
}
```

Запуск кластера:
```shell script
./up-cluster ./gossip-daemon ./processing-daemon-config.json <ip> <ports>...

# к примеру:
./up-cluster ./gossip-daemon ./processing-daemon-config.json 127.0.0.1
```


Запуск обзервера:
```shell script
./gossip-observer ./processing-observer-config.json <ip> <port>
```

Запуск киллера:
```shell script
./killer ./processing-observer-config.json <ip> <port>
```
