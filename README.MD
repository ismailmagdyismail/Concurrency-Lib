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
