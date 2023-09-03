import math
import time
import random

i = 0
while True:
    print(i, random.randint(int(i),int(i)+10),math.sin(i)*100*i*i,math.cos(i)*100*i*i)
    i+=0.01
