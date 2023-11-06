import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

f = 60
t = 1/f
nc = 5

t = np.arange(0,nc*t, t/20)

x = 1*np.sin(2*np.pi*60*t) + 0.4*np.sin(2*np.pi*180*t) + 0.1*np.sin(2*np.pi*240*t)

plt.figure()
plt.plot(t,x)
plt.show()

x = pd.Series(x)