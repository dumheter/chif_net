# What is chif_net?
chif_net is a light cross-platform socket library, one API for socket
functions on Linux, Windows and Mac.

## What parts of sockets are in the library?
TCP and UDP with IPv4 and IPv6 addresses.

## TODO
[ ] Make a multi-socket poll function, (check multiple sockets in one system
call).
[ ] Make a poll function that checks for everything at once. (Can read, can
write, has error).
[ ] Support for UNIX transport protocol.
[ ] Upgrade testing library to latest version of alf_test.
[ ] More extensive tests.
