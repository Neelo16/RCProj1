from socket import *

IP_ADDRESS = '192.168.1.6'

s = socket(AF_INET, SOCK_DGRAM)

host = ('127.0.0.1',58000)
s.sendto('ULQ\n',host)
print 'Waiting for answer'

data = s.recvfrom(1024)
print data
