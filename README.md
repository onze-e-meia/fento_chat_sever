# fento_chat_serv

A simple TCP-based chat server written in C.

## What is this?

A multi-client chat server that:
- Accepts multiple simultaneous TCP connections
- Broadcasts messages from any client to all others
- Announces client arrivals and departures
- Uses select() for non-blocking I/O multiplexing

## Features

- Pure C implementation using POSIX sockets
- No external dependencies
- Handles partial message buffering
- Dynamic memory management
- Clean disconnection handling

## Type

This is a **message relay server** (or **broadcast chat server**),
not a web server. It uses raw TCP sockets, not HTTP.

## Usage
```bash
./fento_chat_serv <port>
nc localhost 4242  # Connect with netcat
```

---
