# robot_comm

This library acts as a communication layer. It supports :

- multiple serializers (Flatbuffers, Protobuf)
- multiple transport layers (Zenoh, tcp, udp, shared memory, ...)

It has been developped with the ambition to support scalability and modularity. Adding a new serializer or tramsport layer is by design meant to be easy.

## Dependencies

- Flatbuffers
- Zenohc
- Zenohcpp
  Need to be built with the following cmake arguments
  ```bash
  -DZENOHCXX_ZENOHC=ON -DZENOHCXX_ZENOHPICO=OFF
  ```
- mc_rtc (used only for logging)
- Protobuf (Optional)

### TODO

- [ ] Consider querier and queryable
- [ ] Do a better management of the processing in serialization. Right now Flatbuffers and Protobuf have been implemented but the serialzation and parsing of the message is still manual since we are converting messages to struct type. It'd be nice to check if it can be generalized. Parsing from or to struct is needed since serializer may not share the same meessage type or generation.
