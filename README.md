# Subscriber/Editor distributed system in C

This project consists in modeling a Kafka-like service using
the real-time service architecture

## Run-up

A script has been implemented that handles the full run-up process: `init.sh`, but you can run this process manually as follows:

1. Export the broker address `host:port` as env variables

    ```bash
    export BROKER_PORT=12345
    export BROKER_HOST=localhost
    ```

2. Compile and run the broker

   ```bash
    cd broker
    make
    ./broker $BROKER_PORT
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

### v0.2

- Client implementations:
  - `send_msg`
  - `msg_length`
  - `end_offset`
  
### v0.3

- Client implementations:
  - `subscribe`
  - `unsubscribe`
  - `position`
  - `seek`
  
### v0.4

- Client implementations:
  - `poll`

### v1.0

- Client implementations:
  - `commit`
  - `committed`
