def splitIns(ins):
    if not ins:
        return

    sepIns = ins.split(" ")
    sepIns = list(filter(None, sepIns))
    fields = {"address" : sepIns[0], "function" : sepIns[1], "ins" : sepIns[3]}
    if fields["ins"] == "call":
        tarAddress = sepIns[4].split("(")[1].strip(")\n")
        tarFunction = sepIns[4].split("(")[0]
        fields["tarAddress"] = tarAddress
        fields["tarFunction"] = tarFunction

    return fields

def main():
    insFile = open("expLog", "r")
    stackSet = []
    funcStack = []
    record = 0
    chain = 0
    callAddress = ""
    addressStack = []
    funcPath = {} 

    ins = insFile.readline()    
    insFields = splitIns(ins)
    while ins:
        if insFields["ins"] == "call":
            record = 1
            if insFields["function"] not in funcStack:
                funcStack.append(insFields["function"])
            addressStack.append(insFields["tarAddress"])
            if insFields["tarAddress"] not in funcPath:
                funcPath[insFields["tarAddress"]] = []
            callAddress = insFields["tarAddress"]

        elif record == 1 and (insFields["function"] not in funcStack):
            funcStack.append(insFields["function"])    
            for address in addressStack:
                funcPath[address].append(insFields["function"])
            record = 0

        elif record == 1 and (insFields["function"] in funcStack):
            if callAddress in funcPath:
                #print("Appending:", address, funcPath[address])
                funcStack.append(funcPath[callAddress])
 
            record = 0           

        elif insFields["ins"] == "ret":
           stackSet.append(funcStack)
           addressStack = []
           funcStack = []

        ins = insFile.readline()    
        insFields = splitIns(ins)

    for stack in stackSet:
        if stack:
            print(stack)
    for key, value in funcPath.items():
        print(key, value)

    insFile.close()
   
main()
'''
                print("----", insFields["function"], "------")
                print(address, funcPath[address])

                print(address, funcPath[address])
                print("-----------------------")
'''
