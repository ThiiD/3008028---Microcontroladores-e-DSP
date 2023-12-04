from scipy.optimize import fsolve
import control
import numpy as np
from matplotlib import pyplot as plt

#Parâmetros
fsw = 20e3
Vcc = 24
L = 2.87e-3
R = 2.37
vcp = 1

cs_gain = 0.104495
rf_acs = 1.7e3
c_filter = 220e-9

kcomp = 1/cs_gain

Tp = 1/(2*np.pi*2e3)
PM = 60
FC = 300

#Cálculo do controlador no domínio da frequência
Gp = control.tf([2*Vcc],[L,R])
Gpwm = control.tf([1],[vcp])
Gs = control.tf([cs_gain],[rf_acs*c_filter,1])
Gc = lambda k,T:control.series(control.tf([k],[1]), control.tf([T, 1], [T,0]), control.tf([1], [Tp, 1]))
Gol = lambda k,T: control.series(Gc(k,T),Gpwm,Gp,Gs,control.tf([kcomp],[1]))

def func(x):
    k = x[0]
    T = x[1]
    if k < 0 or T < 0:
        return [1e10,1e10]
    gm, pm, wcg, wcp = control.margin(Gol(k,T))
    return [pm - PM, wcp - 2*np.pi*FC]


k,T = fsolve(func,[10,0.0001],factor=10,maxfev=10000)
omega = np.logspace(0,5,1000)
control.bode(Gol(k,T),omega,Hz=True,dB=True,deg=True)
gm, pm, wcg, wcp = control.margin(Gol(k,T))
print(f'===== RESULTADOS DO PROJETO DO CONTROLADOR ===========')
print(f'Margem de fase: {pm:.2f}º Frequência de cruzamento: {wcp/(2*np.pi):.2f}')
print(f'Parâmetros: k = {k} | T = {T}')
plt.show()

#Discretização do controlador
fs = fsw
controller = control.sample_system(Gc(k,T), 1/fs, method='bilinear')
print(f'===== CONTROLADOR NO DOMÍNIO DISCRETO ===========')
print(controller)

output = ''
order = max(len(controller.num[0][0]),len(controller.den[0][0]))
for i in range(order):
    output += f"#define B{i} {controller.num[0][0][i]/controller.den[0][0][0]:.6f}\r\n"
for i in range(1,order):
    output += f"#define A{i} {controller.den[0][0][i]/controller.den[0][0][0]:.6f}\r\n"
print(f'===== COEFICIENTES DA EQUAÇÃO DE DIFERENÇAS ===========')
print(output)
 