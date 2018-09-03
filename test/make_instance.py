import os
import sys
import json 
import random

def vname(s): return 'v_'+str(s)
def ename(a,b): return 'e_'+str(a)+'_'+str(b)

net = {'rows':[], 'controllers':[]}

p = float(sys.argv[2])
n = int(sys.argv[3])

if n<2:
    print( 'n needs to be at least 2')
    exit()

for u in range(0,n-1):
    net['rows'].append({'viaGlobalId': ename(u,u+1),'fromGlobalId': vname(u),'toGlobalId': vname(u+1)})
    for v in range(u+2,n):
        if random.random()<p:
            net['rows'].append({'viaGlobalId': ename(u,v),'fromGlobalId': vname(u),'toGlobalId': vname(v)})

controller = random.randint(0,n-1)
net['controllers'].append({'globalId':vname(controller)})

start = controller
while start==controller: start = random.randint(0,n-1)

instance = sys.argv[1] + '_' + str(int(100*float(sys.argv[2]))) + '_' + sys.argv[3]
if os.path.exists(instance):
    print('"' + instance + '" already exists.')
else:
    os.mkdir(instance)
os.chdir(instance)
with open('network.json','w') as f: json.dump(net,f)
with open('start.txt','w') as f: f.write(vname(start)+'\n')
with open('expected.txt','w') as f: f.write("random instance\n")
