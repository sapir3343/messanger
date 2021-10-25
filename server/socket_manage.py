from config import *

# def read(conn):
#     """
#     read data from ready socket
#     return all data that send
#     """
#     new_data = conn.recv(MAX_PACK_SIZE)  
#     data = new_data
#     while len(new_data) == MAX_PACK_SIZE:
#         new_data = conn.recv(MAX_PACK_SIZE)  
#         data += new_data
    
#     return data

def read(conn, size):
    """
    read data from ready socket
    param size - byte size to read from socket
    return all data that send
    """
    data = self._conn.recv(size)  
    
    return data
