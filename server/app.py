from logging import log
from protocol_handler import ProtocolHandler
import selectors
import socket
from socket_manage import read
import types
from config import *
from logger import logging
import db_connector as db
# import request_processor as req_p
# import response_handler as res_h
# from constants import *
import struct

def accept_client(sock, sel, db_connection):
    """
    accept client connection
    """
    conn, addr = sock.accept() 
    logging.info(f'new connection from {addr}')
    conn.setblocking(False)
    sel.register(conn, selectors.EVENT_READ, handle_client_connection)

def handle_client_connection(conn, sel, db_connection):
    ProtocolHandler(conn, sel, db_connection).handle_request()

def main():
    # create a database manager
    db_connection = db.DBConnection()
    db_connection.init_db()

    # get port number to listen on from file
    with open("port.info", "r") as file:
        try:
            port = int(file.read())

            if port > MAX_PORT_NUMBER or port < MIN_AVAILABLE_PORT:
                raise ValueError
        except:
            logging.fatal("port.info format is invalid")
            exit()

    host = HOST

    # create a selector and a socket
    selector = selectors.DefaultSelector()
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # start server
    sock.bind((host, port))
    sock.listen(MAX_CONNECTION)
    print('server listening on', (host, port))

    sock.setblocking(True)
    selector.register(sock, selectors.EVENT_READ, data=accept_client)

    # server run until brutal exit 
    while True:
        events = selector.select(timeout=None)
        for key, mask in events:
            
            for key, mask in events:
                callback = key.data
                logging.info("calling : " + str(callback))
                callback(key.fileobj, selector, db_connection)
                        
if __name__ == '__main__':
    main()