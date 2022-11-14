import subprocess
import time
import os
# La 3 esta incompleta
numprocesos = [1, 2, 3, 4, 5, 6, 9, 12, 18]
maxprocesos = 18

directorioInicial = os.path.curdir

def sendScript(script, npr, nth) :  
    abierto = False

    while not abierto :
        try :
            f = open("../input/in.txt", "w")
            f.write("2000 256 "+str(npr)+" "+str(nth)+"\n")
            f.close()
            abierto = True
            pass
        except IOError:
            print("Fallo al abrir in.txt, reintentando...")
            abierto = False
            time.sleep(10)
            pass
    
    print("qsub "+script+" con: "+str(npr)+" - "+str(nth))
    subprocess.call(["qsub", script])

def sendCuestion(npr, nth) :
    
    print(str(npr)+": "+str(nth))
    sendScript("script.pbs", npr, nth)
    time.sleep(250)

    print("mv ../output/out.txt ../output/"+str(npr)+"_"+str(nth)+".txt")
    subprocess.call(["mv", "../output/out.txt", "../output/"+str(npr)+"_"+str(nth)+".txt"])

for i in numprocesos :
    nth=18/i
    sendCuestion(i, nth)
    