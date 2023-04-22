# Subscriber/Editor distributed system in C

This project consists in modeling a Kafka-like service using
the real-time service architecture

- Broker: Server that hosts the various message queues a.k.a `topics`:
- Topic: Abstract class that allows for cluster modeling and concurrency, `clients` or `subscribers` subscribe to any of these topics