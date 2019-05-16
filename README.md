# What is chif_net?
chif_net is a light cross-platform socket library, aiming to provide
a unified API on Windows, Mac and Linux for the commonly used socket
functionality.


## What parts of sockets are in the library?
For now, IP protocols TCP, UDP, ICMP and RAW are supported. They can be trasfered on IPv4 connections or IPv6 connections.

## TODO
[ ] Make a multi-socket poll function, (check multiple sockets in one system
call).
[ ] Make a poll function that checks for everything at once. (Can read, can
write, has error).
[ ] Enable the UNIX transport protocol.
[ ] Upgrade testing library to latest version of alf_test.
