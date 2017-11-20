# 客户端代码，环境Mac
#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import socket

SIZE = 1024

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 建立连接:
s.connect(('127.0.0.1', 9999))
# 接收欢迎消息:
print s.recv(SIZE)
s.send('begin to send')
print 'sending, please wait for a second ...'
with open('./image.bmp', 'rb') as f:
    for data in f:
        s.send(data)
print 'sended !'
s.close()
print 'connection closed'

# 作者：eric_lai
# 链接：http://www.jianshu.com/p/2a4b859e05df
# 來源：简书
# 著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。