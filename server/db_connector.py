from exceptions import FailedPullMessages
import logging
import sqlite3
from logger import logging
from config import *


class Client:
    def __init__(self, id, name, public_key, last_seen):
        self.id = id
        self.name = name
        self.public_key = public_key
        self.last_seen = last_seen

    def __str__(self):
        return f'client:\n id: {self.id}, name: {self.name}\npublic key: {self.public_key},' \
               f'last seen: {self.last_seen}'
    
    def __eq__(self, other): 
        if not isinstance(other, Client):
            return NotImplemented

        return (self.id == other.id and 
                self.name == other.name and
                self.public_key == other.public_key)

class Message:
    def __init__(self, to_client, from_client, message_type, content, id=None):
        self.id = id
        self.to_client = to_client
        self.from_client = from_client
        self.message_type = message_type
        self.content = content

    def __str__(self):
        return f'message:\n id: {self.id}, send to: {self.to_client}\nsent from: {self.from_client}, ' \
               f'type: {self.message_type}\ncontent:\n{self.content}'

class DBConnection:

    def __init__(self):
        self.db_connection = None
        try:
            # DB connection or creation
            self.db_connection = sqlite3.connect('db.server')
            self.db_connection.text_factory = bytes
            # create cursor for select queries
            self.cursor = self.db_connection.cursor()

                
        except Exception as e:
            logging.exception('an error has happened while creating the DB', e) 

    def init_db(self):
        self.cursor.executescript(f"""
                CREATE TABLE IF NOT EXISTS 
                    clients(ID varchar(16) NOT NULL PRIMARY KEY,
                            Name varchar({NAME_LENGTH}) NOT NULL,
                            PublicKey varchar({PUBLIC_KEY_LENGTH}) NOT NULL, 
                            LastSeen text);
                CREATE TABLE IF NOT EXISTS 
                    messages(ID INTEGER PRIMARY KEY AUTOINCREMENT,
                            ToClient varchar(16) NOT NULL, 
                            FromClient varchar(16) NOT NULL,
                            Type CHAR NOT NULL, 
                            Content blob); """)
        self.db_connection.commit()
        

    def close_connection(self):
        self.db_connection.close()

    
    def get_client(self, client_id):
        """
        select client information from db by id
        param client_id: id
        return Client object
        """
        try:
            self.cursor.execute("SELECT * FROM clients WHERE ID =?", [client_id])
            result = self.cursor.fetchall()
            self.db_connection.commit()
        except Exception as e:
            logging.exception(f'an error occurred while trying \
                                to get client with id: {client_id}', e)
            return None

        # for none exists client
        if not result:
            return None

        # create Client object
        client_row = result[0]
        id = client_row[0]
        name = client_row[1]
        public_key = client_row[2]
        last_seen = client_row[3]
        client = Client(id, name, public_key, last_seen)
        return client

    
    def add_client(self, client: Client):
        """
        add new client to db
        return True if successfully added False otherwise
        """
        try:
            query = 'INSERT INTO clients(ID, Name, PublicKey, LastSeen) VALUES(?,?,?,?)'
            params = (client.id, client.name, client.public_key, client.last_seen)

            self.cursor.execute(query, params)
            self.db_connection.commit()
            return True
        except Exception as e:
            logging.exception(f'error occurred while trying to add client - {client}', e)
            return False

    def get_all_clients(self):
        """
        get list of all client from DB 
        return list of all clients Client objects
        return None if exception has been raised 
        """
        try:
            results = []
            query_result = self.cursor.execute('SELECT * FROM clients').fetchall()
            self.db_connection.commit()
        except Exception as e:
            logging.error('failed to get clients list')
            return

        # create client object for each row
        for row_client in query_result:
            results.append(Client(row_client[0], row_client[1], row_client[2], row_client[3]))
        
        return results

    def add_message(self, message: Message):
        """
        add message to the DB ,
        return message id if success
        otherwise return None
        """
        try:
            query = 'INSERT INTO messages(ToClient,FromClient,Type,Content) VALUES(?,?,?,?)'
            params = (message.to_client, message.from_client,
                    message.message_type, message.content)

            self.cursor.execute(query, params)
            self.db_connection.commit()

            # returns message id
            return self.cursor.lastrowid
        except Exception as e:
            logging.exception(f'error occurred while adding message with this record: {message}', e)
            return None
 
    def delete_messages(self, client_id):
        """
        delete all client_id waiting messages 
        return True if success False otherwise
        """
        try:   
            self.cursor.execute("DELETE FROM messages WHERE ToClient = ?", [client_id])
            self.db_connection.commit()
            return True
        except Exception as e:
            logging.error(f"failed delete {client_id} messages", e)
            return False           

    def get_all_messages_for_client(self, client_id):
        """
        return all messages for specific client
        remove all this messages 
        return None if there were problems
        """
        try:
            # retrieve all the message that need to be pulled
            self.cursor.execute("SELECT * FROM messages WHERE ToClient = ?", [client_id])
            query_results = self.cursor.fetchall()
            self.db_connection.commit()
        except Exception as e:
            logging.exception(f'error occurred while get messages for: {client_id}', e)
            return None

        # delete them from the DB
        if not self.delete_messages(client_id):
            raise FailedPullMessages("failed deleted recived messages")

        result = []
        for row in query_results:
            message = Message(id=row[0], to_client=row[1],
                                        from_client=row[2],
                                        message_type=row[3], content=row[4])
            result.append(message)
        return result
