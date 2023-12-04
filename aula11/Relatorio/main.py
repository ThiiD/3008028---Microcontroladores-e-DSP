import matplotlib.pyplot as plt
from control.matlab import *

num = [1.14668610e+07]
den = [1, 6.77255077e+03, 1.14668610e+07]

G = tf(num, den)
Gf = series(G,G)
bode(G)
plt.savefig("figuras/bode.pdf", bbox_inches = "tight")
plt.show()