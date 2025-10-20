# MQTT-2-PSQL

A service to listen to an MQTT broker for messages from IoT sensors and
write them to a PostgreSQL database.


## Building

Dependancies:

- make
- g++
- libpqxx-dev
- nlohmann-json3-dev

Optional:
- systemd

To build the binary:

    make

To build the binary without systemd:

    NO_SYSD=1 make

The binary will be `build/bin/mqtt2psql`. An example config file can
be located at `pkg/etc/mqtt2psql/mqtt2psql.conf`.


## Packaging

Debian packaging has been added to the makefile:

    make deb
    make deb-lint  # for linting your .deb

The .deb file will be `build/mqtt2psql_VERSION_ARCH.deb` in the classic
debian naming format.


## Installing

Once you have the .deb file, you can install it on a debian based
distribution with:

    apt install -y ./build/mqtt2psql_VERSION_ARCH.deb

Modify the config file at `/etc/mqtt2psql/mqtt2psql.conf` to match your
setup.

Enable and start the service

    systemctl enable --now mqtt2psql  # With systemd
    service mqtt2psql start  # Without systemd


License: see License file
