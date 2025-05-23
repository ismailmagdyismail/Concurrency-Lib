# Concurrency Primitives

- Provide basic building blocks for building Concurrent applications
- helps decouple threading aspects from bussniess logic itself ,acheiving Separation of Concerns
- provide a way to write busniess code in a synchronus way , while those primitves handle Concurrency aspect

## Channels

- Inspired by Go Channels
- Inspired by CSP (Communicating Sequential processes) paper
- they can be though of as a Message Queue (of length 1 for UnBufferedChannels)
- they apply CSP patterns
- they provide a way to acheive "Communication is Synchronization"
- they provide a way to acheive "Don't communicat by sharing data, instead share data by Communicating"
- building blocks for "Message Passing" Mechanisms
- Help in building "Actor" Models / patterns more easily

#### UnBufferedChannels

- A Thread safe, lock based Synchronization point
- Helps Synchronize Threads Communicating, Providing a Medium to exachange and store messages
- Each Send , Recieve API call is Blocking / Synchronous; blocks till a message is consumed, and message is available respectivly
- Could be treated like Message Queue/ BufferedChannel of Size (1)

#### BufferedChannels

- a Message Queue for storing multiple Messages
- Support Multiple Producers and multiple Consumers
- Works in an Async way , producers don't have to wait for result , they publish message and continue doing there work

#### ChannelSelector

- Like Go's Select statement
- provide a way to select from multiple channel sources (of same or diff data types)
- Helpful when Channel Sources don't recieve Data at same rate (one may be much slower than other )
- Waiting on multiple channels Might cause a deadlock if the first channel doesn't ever recieve data!, it also may be wasting bandwidth if the other channels have data already ready waiting for them to be consumed so that another data can be produced. ChannelSelector Helps solves this
- It also support listening on diff data types, which would be done with Variatns or void\* or Polymorphism if you want to do it on same channel

## Thread

- a worker thread , that runs forever till destroyed
- avoid re-creating threads , those ones can be reused
- provide joining in destructor to avoid std::terminate expection (RAII)

## Actors

- represent simple actor based pattern
- each actor runs in its own thread
- they can be paused and resumed and stopped permenantly
