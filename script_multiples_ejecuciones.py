import subprocess
import time
import os
# La 3 esta incompleta
cuestiones = ["Cuestion2", "Cuestion3", "Cuestion4", "Cuestion5"]
num = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18]

directorioInicial = os.path.curdir

def sendScript(script, n) :
    nproc = str(n)      
    abierto = False

    while not abierto :
        try :
            f = open("../input/in.txt", "w")
            f.write("5000 500 "+nproc+"\n")
            f.close()
            abierto = True
            pass
        except IOError:
            print("Fallo al abrir in.txt, reintentando...")
            abierto = False
            time.sleep(10)
            pass
    
    print("qsub "+script)
    subprocess.call(["qsub", script])

def sendCuestion(cuestion, num) :
    directorio = cuestion+"/src/"
    os.chdir(os.path.join(os.path.abspath(directorioInicial),directorio)) # Esto no funciona bien a partir de la segunda iteracion

    for n in num :
        print(cuestion+": "+str(n))
        sendScript("script2.pbs", n)
        time.sleep(250)

    print("mv ../output/out.txt ../output/venus+marte.txt")
    subprocess.call(["mv", "../output/out.txt", "../output/venus+marte.txt"])

for cuestion in cuestiones :
    directorioInicial = os.path.curdir
    sendCuestion(cuestion, num)
    