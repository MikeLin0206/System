import os
import sys

def searchFunc(symTable, function):
    entry = symTable.readline()
    while entry:
        entry = entry.strip("\n")
        info = entry.split(" ")
        while '' in info:
            info.remove('')

        if len(info) == 3: 
            if info[2] == function and  info[1] == "T" :
                return "T"
        else: 
            if info[1] == function and info[0] == "U":
                return "U"
        entry = symTable.readline()

    return "N"

def getSymTable(fileName):
    symTable = os.popen("nm {}".format(fileName))
    entry = symTable.readline()
    if "no symbol" in entry:
       symTable = os.popen("nm -D {}".format(fileName))

    return symTable

def getLibPath(library):
    library = os.popen("ldconfig -p | grep {}".format(sys.argv[2]))
    entry = library.readline()
    library = entry.split("=>")[1].strip("\n").strip(" ")

    return library

def isRecord(function):
    discardFile = open("discard", "r")
    funcFile = open("funcList", "r")
        
    disList = discardFile.readline()
    funcList = funcFile.readline()
    discardFile.close()
    funcFile.close()

    function = " " + function + " "

    if function in disList:
        print("False")
        return True

    elif function in funcList:
        print("True")
        return True

    return False

def main():
    if isRecord(sys.argv[1]):
        return

    symTable = getSymTable(sys.argv[3]) 
    routineType = searchFunc(symTable, sys.argv[1])
    disFile = open("discard", "a")
    funcFile = open("funcList", "a")

    if routineType == "T":
        funcFile.write((sys.argv[1] + " ").rstrip("\n")) 
        print("True")

    elif routineType == "U":
    #    print(sys.argv[0], sys.argv[1],sys.argv[2], sys.argv[3])
        library = getLibPath(sys.argv[2])
        symTable = os.popen("nm -D {}".format(library))
        routineType = searchFunc(symTable, sys.argv[1])
        if routineType == "T":
            funcFile.write((sys.argv[1] + " ").rstrip("\n")) 
            print("True")
        else:
            disFile.write((sys.argv[1] + " ").rstrip("\n")) 
            print("False")

    else:
        disFile.write((sys.argv[1] + " ").rstrip("\n")) 
        print("False")

    disFile.close()
    funcFile.close()

main()
