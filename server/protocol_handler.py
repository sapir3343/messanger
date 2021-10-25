from logging import exception
import selectors
from db_connector import Client, DBConnection, Message
import socket
import struct
import uuid
from datetime import datetime
from config import *
from socket_manage import read
from logger import logging
from exceptions import ProtocolError, UnkownClient, UnkownCode, VersionError

class ProtocolHandler:
    def __init__(self, conn: socket.socket, selector: selectors.SelectSelector, db_connection: DBConnection):
        self._conn = conn
        self.selector = selector
        self.db_connection = db_connection
        self.target_address = conn.getpeername()
        self._header_length = 23
        self.message_heafer_length = 21
        self.__VER__ = VERSION
        self.__UID_LENGTH__ = UID_LENGTH
        self.__PUBLIC_KEY_LENGTH__= PUBLIC_KEY_LENGTH
        self.__NAME_LENGTH__ = NAME_LENGTH
        self.__MAX_PACKAGE__ = MAX_PACK_SIZE
        self._requests_codes = {1000: self.register, 1001: self.users_list, 
                                 1002: self.get_user_public_key, 1003: self.send_message,
                                 1004: self.pull_messages}
        self._response_codes = {'register_succeeded': 2000, 'return_user_list': 2001,
                                 'return_user_key': 2002, 'message_send': 2003, 
                                 'messages_pulled': 2004, 'error': 9000}
        self.messages_ids = {1: "get_key", 2: "send_key", 3: "send_text",  4: "send_file"}

        
    def read(self, size):
        """
        read data from ready socket
        param size - byte size to read from socket
        return all data that send
        """
        data = b""
        if size > MAX_PACK_SIZE:
            new_data = self._conn.recv(MAX_PACK_SIZE) 
            size = size - MAX_PACK_SIZE 
            data += new_data
            while size > MAX_PACK_SIZE:
                new_data = self._conn.recv(MAX_PACK_SIZE)  
                data += new_data
                
        try:
            data += self._conn.recv(size)
            if len(data) == 0 :
                return CONNECTION_CLOSED
            elif len(data) < size:  
                raise ProtocolError("packet recived is too short")
        except Exception as e:
            pass
            
        return data

    def handle_request(self):
        """
        read data from ready socket and handle the request, if valid.
        raise an exception if connection is broken, big message or 
        data is not according to the protocol
        """          
        try:
            data = self.read(self._header_length)
            if data == CONNECTION_CLOSED:
                logging.info(f"client {self.target_address} close connection")
                if self._conn:
                    self.selector.unregister(self._conn)
                    self._conn.close()
                return

            header_fields = struct.unpack('<' + str(self.__UID_LENGTH__) + 'sBHI', data[:self._header_length])
            user_id, version, code, payload_size = header_fields
            logging.info("header packet received - " + str(header_fields))
            
            if code not in self._requests_codes.keys():
                raise UnkownCode("not supported code - ", code)

            if version > CLIENT_MAX_VERSION:
                raise VersionError("not supportes client version - ", version)

            if code == REGISTER_CODE:
                client = None
            else:
                client = self.db_connection.get_client(user_id)
                if not client:
                    raise UnkownClient("unauthorized client")

            logging.info("read rest of packet")    
            payload = self.read(payload_size)
            self._requests_codes[code](client, payload, payload_size)

        except Exception as e:
            self.send_response(self._response_codes['error'])
            logging.error("general error")      
            logging.error(e)    

    def send_response(self, response_code, payload=None):
        """ 
        send response to client in socket accordind to protocol\
        payload should be struct correlate to response protocol
        return True if success false otherwise
        """
        
        message = struct.pack('<B H', self.__VER__, response_code)  # pack the version and the respond code
        if not payload:
            message += struct.pack('<I', 0)
        else:
            message += struct.pack('<I', len(payload))
            message += payload
        
        try:
            self._conn.send(message)
            logging.info(f"send {response_code} response to {self.target_address}")
            return True
        except ConnectionResetError:
            logging.info(f"client {self.target_address} close connection")
            return False
                                                                   
    def register(self, client, data, payload_size):
        """
        register new user
        callback of handle request - get client, data and payload_size
        param data - struct of name and publickey recived from socket
        param payload size - data size in bytes
        """
        payload_max_size = self.__PUBLIC_KEY_LENGTH__ + self.__NAME_LENGTH__
        payload_min_size = self.__PUBLIC_KEY_LENGTH__ + 1

        try: 
            # assert error if register payload size is not correlate to protocol
            assert ( payload_max_size >= payload_size > payload_min_size )
            fields = struct.unpack('<' + str(payload_size - self.__PUBLIC_KEY_LENGTH__) + 's' 
                    + str(self.__PUBLIC_KEY_LENGTH__) + 's', data)
            
            name, public_key = fields
            user_id = uuid.uuid4().bytes
            client = Client(user_id, name, public_key, datetime.now())
           
            if not self.db_connection.add_client(client):
                logging.error("error in register")
                self.send_response(self._response_codes['error'])
            
            payload = struct.pack(f"<{self.__UID_LENGTH__}s", user_id)

            self.send_response(self._response_codes['register_succeeded'], payload)
            logging.info("client " + name.decode("utf-8")  + " register successufully")
            

        except (AssertionError, struct.error) as e:
            raise ProtocolError("register payload not match protocol")

    def users_list(self, client, data, payload_size):
        """
        return to client user lists from server except his record
        callback of handle request - get client, data and payload_size
        client object of client send the message
        param data - not relevant
        param payload size - not relevant
        """
        if payload_size > 0: 
            raise ProtocolError("no data excepted")
        
        clients = self.db_connection.get_all_clients()
        clients.remove(client)

        payload = b""

        for client_row in clients : 
            payload += struct.pack(f"<{self.__UID_LENGTH__}s{self.__NAME_LENGTH__}s", 
                                    client_row.id, 
                                    client_row.name)
        self.send_response(self._response_codes["return_user_list"], payload)

    def get_user_public_key(self, client, data, payload_size):
        """
        return to client public key of specific client id
        callback of handle request - get client, data and payload_size
        param data - client id of wanted user
        client object of client send the message
        """
        try:        
            fields = struct.unpack(f'{self.__UID_LENGTH__}s', data)
            user_id = fields[0]
            wanted_client = self.db_connection.get_client(user_id)
            payload = struct.pack(f"<{self.__UID_LENGTH__}s{self.__PUBLIC_KEY_LENGTH__}s", 
                                    user_id, wanted_client.public_key)

            self.send_response(self._response_codes['return_user_key'], payload)
            
        except struct.error as e:
            raise ProtocolError("get public key payload not match protocol")

    def send_message(self, client, data, payload_size):
        """
        handle request of send message to other client
        send client uniqe message id that sent.
        callback of handle request - get client, data and payload_size
        param data - message type and content to send
        param payload size - how much data to read
        param client - object of client send the message
        """
        content = b""
        try:        
            fields = struct.unpack(f'<{self.__UID_LENGTH__}sBI', data[:self.message_heafer_length])
            client_dst_id, message_type, content_size = fields

            if content_size > 0: 
                content = data[self.message_heafer_length:]

            wanted_client = self.db_connection.get_client(client_dst_id)
            if not wanted_client or not message_type in self.messages_ids:
                raise ProtocolError("try to send message to unkown client")

            message_id = self.db_connection.add_message(Message(wanted_client.id,
                                                                client.id, message_type,
                                                                content))
         
            payload = struct.pack(f"<{self.__UID_LENGTH__}sI", 
                                    wanted_client.id, message_id)

            self.send_response(self._response_codes['message_send'], payload)
            logging.info("message from client " + str(client.id) + " received")            
        except struct.error as e:
            raise ProtocolError("send message payload not match protocol")

    def pull_messages(self, client, data, payload_size):
        """
        return to client all message waited for him
        callback of handle request - get client, data and payload_size
        param client - client object of client send the message
        param data - not relevant
        param payload size - not relevant
        """
        if payload_size > 0: 
            raise ProtocolError("no data excepted")
        
        messages = self.db_connection.get_all_messages_for_client(client.id)

        payload = b""

        for message in messages : 
            message_size = len(message.content)
            payload += struct.pack(f"<{self.__UID_LENGTH__}sIBI", 
                                    message.from_client, 
                                    message.id,
                                    int(message.message_type),
                                    message_size)
            if message_size > 0: 
                payload +=  struct.pack(f"<{message_size}s", message.content)
        

        self.send_response(self._response_codes['messages_pulled'], payload)
            
        
    


