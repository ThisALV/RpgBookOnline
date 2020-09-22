import socket

host = "localhost"
port = 50505


class Player(object):
    def __init__(self, id, name):
        if type(id) != int or type(name) != str or id < 0 or id > 255:
            raise ValueError("Id: byte, Name: string  attendus")

        self.connection = socket.create_connection((host, port))
        self.connection.send(id.to_bytes(
            1, "big", signed=False) + name.encode() + b"\x00")

    def ready(self):
        self.byte(0)

    def leave(self):
        self.byte(1)

    def byte(self, byte):
        self.connection.send(byte.to_bytes(1, "big", signed=False))

    def get(self):
        return self.connection.recv(4096)
