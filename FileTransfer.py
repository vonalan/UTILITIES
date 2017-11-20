#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import socket, threading, os

SIZE = 1024

class Server(object): 
    def __init__(self, host='127.0.0.1', port=9999, max_num_clients=1): 
        self.host = host 
        self.port = self._get_valid_port(port)
        self.max_num_clients = max_num_clients
        self.run()
    def _get_valid_port(self, port=9999): 
        return port 
    def _checkFile(self):
        list = os.listdir('.')
        for iterm in list:
            if iterm == 'image.bmp':
                os.remove(iterm)
                print 'remove'
            else:
                pass
    def _tcplink(self, sock, addr):
        print 'Accept new connection from %s:%s...' % addr
        sock.send('Welcome from server!')
        print 'receiving, please wait for a second ...'
        while True:
            data = sock.recv(SIZE)
            if not data :
                print 'reach the end of file'
                break
            elif data == 'begin to send':
                print 'create file'
                self._checkFile()
                with open('./image.bmp', 'wb') as f:
                    pass
            else:
                with open('./image.bmp', 'ab') as f:
                    f.write(data)
        sock.close()
        print 'receive finished'
        print 'Connection from %s:%s closed.' % addr
    def run(self): 
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((self.host, self.port))
        s.listen(self.max_num_clients)
        print 'Waiting for connection...'
        while True:
            sock, addr = s.accept()
            t = threading.Thread(target = self._tcplink, args = (sock, addr))
            t.start()

class Client(object): 
    def __init__(self, host='127.0.0.1', port=9999): 
        self.run(host, port)
    def run(self, host='127.0.0.1', port=9999): 
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        print s.recv(SIZE)
        s.send('begin to send')
        print 'sending, please wait for a second ...'
        with open('./image.bmp', 'rb') as f:
            for data in f:
                s.send(data)
        print 'sended !'
        s.close()
        print 'connection closed'
    def transfer(self, filepath, host='127.0.0.1', port=9999): 
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        print s.recv(SIZE)
        s.send('begin to send')
        print 'sending, please wait for a second ...'
        # with open('./image.bmp', 'rb') as f:
        #     for data in f:
        #         s.send(data)
        with open(filepath, 'r') as rf: 
            for data in rf: 
                s.send(data)
        print 'sended !'
        s.close()
        print 'connection closed'

if __name__ == '__main__': 
    host = ''
    port = ''
    client = Client(host, port)

    video_dir = ''
    video_list = os.listdir(video_dir)
    for video_name in video_list: 
        video_path = os.path.join(video_dir, video_name)
        client.transfer(video_path, host, port)