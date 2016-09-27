from socket import *

s = socket(AF_INET,SOCK_STREAM)
s.bind(('localhost',58001))
s.listen(1)

s2 = socket(AF_INET,SOCK_DGRAM)
s2.bind(('localhost',58002))
print 'trying to register...'
s2.sendto('SRG Ingles localhost 58001',('localhost',58000))
data,sender = s2.recvfrom(1024)
print data
while 1:
	try:
		conn,addr = s.accept()
		data = conn.recv(1024)
		print data
		if 'TRQ t' in data:
			conn.send('TRR t'+data[5:])
		elif 'TRQ f' in data:
			conn.send('TRR f pato 10 goodenough')
	except:
		break
s.close()
s2.close()