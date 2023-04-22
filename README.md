# Subscriber/Editor distributed system in C

This project consists in modeling a Kafka-like service using
the real-time service architecture

## Run-up

1. Export the broker address `host:port` as env variables

    ```bash
    export BROKER_PORT=12345
    export BROKER_HOST=localhost
    ```

2. Compile and run the broker

   ```bash
    cd broker
    make
    ./broker BROKER_PORT
   ```

3. Compile and run the clients

   ```bash
    cd clients
    make
    ./test
   ```

## Structs

- Broker: Server that hosts the various message queues a.k.a `topics`:
- Topic: Abstract class that allows for cluster modeling and concurrency, `clients` or `subscribers` subscribe to any of these topics
- Client: Light programms that connect to the broker to get real-time updates from the queues/topics they're intereseted in

## Version control

### v0.1

- Client implementations:
  - `create-topic()`
  - `ntopics`
