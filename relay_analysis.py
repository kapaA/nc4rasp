



e1 = 0.3
e2 = 0.1
e3 = 0.5

k = list()
import numpy as np
gen = 100
ex = 0;

for i  in range (1, gen):


	apx = (float(i) * (1 - e1))*(( (e3 * (1- e2)) - 1)) - (1 - e1)
	p = 256 ** apx
	k.insert(i ,p ) 
	
	ex = ex +  i *p 

print "expected value"
print ex
			

import matplotlib.pyplot as plt

x = np.arange(1,gen ,1)

plt.plot(x,k)

plt.legend(['with helper', 'without helper', 'Analysis PlayNCool'], loc='upper left')
plt.xlabel('e_2')
plt.ylabel('total number of transmission')
plt.savefig('tx.pdf')
plt.show()

print k

