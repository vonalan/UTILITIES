#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# 导入socket库
import socket, threading, time, os

SIZE = 1024

# 检查当前目录下是否有等下要命名的图片,有的话删除之
def checkExsit():
    list = os.listdir('.')
    for iterm in list:
        if iterm == 'image.bmp':
            os.remove(iterm)
            print 'Exsit file has been removed'
    print 'Create file ...'
    with open('./image.bmp', 'wb') as f: pass

def recvImage(sock):
    while True:
        data = sock.recv(SIZE)
        if not data:
            break
        else:
            with open('./image.bmp', 'ab') as f:
                f.write(data)
    print 'data received'

def saveImage(sock):
    print 'Begin to save image ...'
    checkExsit()
    t = threading.Thread(target = recvImage, args = (sock,))
    t.setDaemon(True)
    t.start()
    t.join()
    print 'Finished saving image ...'

def tcplink(sock, addr):
    # 打印连接信息
    print 'Accept new connection from %s:%s...' % addr
    # 发送问候信息(客户端接收到后返回一个'hello server')
    sock.send('hello client')
    print sock.recv(SIZE)
    print 'Communication test success'
    # 接受数据循环（一直等待接收数据并进行处理 *****注意这是在一个线程里面******）
    while True:
        recv = sock.recv(SIZE)
        # 接收命令
        if recv == 'c':
            print 'receive command'
            cmd = sock.recv(SIZE)
            print 'recv: %s' %cmd
            # 判断命令并执行相应的程序
            recv = None
        # 接收文件（这里主要是图片）
        elif recv == 'f':
            print 'file command'
            saveImage(sock)
            recv = None

# 创建一个socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 监听端口（这里的ip要在不同的情况下更改）
s.bind(('127.0.0.1', 9999))
# 每次只允许一个客户端接入
s.listen(1)
print 'Waiting for connection...'
while True:
    sock, addr = s.accept()
    # 建立一个线程用来监听收到的数据
    t = threading.Thread(target = tcplink, args = (sock, addr))
    # 线程运行
    t.start()

# 作者：eric_lai
# 链接：http://www.jianshu.com/p/2a4b859e05df
# 來源：简书
# 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。