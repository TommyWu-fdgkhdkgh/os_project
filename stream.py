import numpy as np
import cv2
import socket
import sys

if len(sys.argv)<4:
    print('usage : [ ip ] [ port ] [ file name ]')
    sys.exit()
ip=sys.argv[1]
port=int(sys.argv[2])
sock=socket.socket()
sock.connect((ip,port))

name=sys.argv[3]
sock.send(name.encode('utf-8'))

capture=cv2.VideoCapture(name)
success , frame = capture.read()
c=0
maxvalue = 0
while(success):
    c+=1
    print(c)
    _,imgencode=cv2.imencode('.jpg',frame,[int(cv2.IMWRITE_JPEG_QUALITY),90])
    data=np.array(imgencode)
    strdata=data.tostring()
    sock.send(str(len(strdata)).ljust(16).encode('utf-8'))
    print(str(len(strdata)).ljust(16).encode('utf-8'))
    
    #if len(strdata) > maxvalue:
    #    maxvalue = len(strdata)

    sock.send(strdata)
    #print(strdata)

    success , frame = capture.read()
    cv2.waitKey(30)

    #if c == 5 :
    #    break;

#print("maxvalue:" + str(maxvalue))

capture.release()
cv2.destroyAllWindows()
