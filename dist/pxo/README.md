These are the default files for multiplayer using the new PXO.

For FreeSpace there are two config files: `pxo.cfg` and `std.cfg`. The `pxo.cfg`
file contains PXO server connection information and the `std.cfg` file contains
settings for the standalone server.

For FreeSpace 2 the config file is `multi.cfg`.

The `+pxo` setting in `multi.cfg`/`std.cfg` determines if PXO is to be enabled
for a standalone server and in what chat channel to advertise the server. A
setting of `+pxo #Eleh` will enable PXO and advertise the game in the #Eleh chat
channel. Using a special chat channel of `global` will cause the server to be
visible for all chat channels.

Removing the `+pxo` line or changing the + to a - (i.e. `-pxo`) will disable the
use of PXO for a standalone server and switch it to LAN mode instead.

These files have settings optimizing play for LAN games or PXO games over an
internet connection of 10 Mbps or greater.