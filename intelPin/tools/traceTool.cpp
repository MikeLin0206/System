#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <execinfo.h>
#include <vector>
#include "pin.H"

using namespace std;

ofstream OutFile;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "inscount.out", "specify output file name");
typedef struct RtnCount{
    string _name;
    string _nextName;
    string _image;
    string _progName;
    ADDRINT _address;
    RTN _rtn;
    UINT64 _rtnCount;
    UINT64 _icount;
    INS insHead;
    INS insTail;
    struct RtnCount * _next;
} RTN_COUNT;
RTN_COUNT * RtnList = 0;

typedef struct Node{
    string routine;
    Node *prev;
    vector<Node> branch;
}Node;

struct callPathArray{
    vector<Node> pathArray;
};

const char * StripPath(const char * path){
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

string exec(const char* cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = popen(cmd, "r");
    while(fgets(buffer, sizeof buffer, pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}
int isTargetFunction(RTN_COUNT *rtnCount){
    string command = "python3 ~/exeTest/getFunc.py " + rtnCount -> _name + " " + rtnCount -> _image;
    command += " " + rtnCount -> _progName;
    string result;
    const char* a = command.c_str();
    result = exec(a);
    if(result == "True\n") return 1;
    else return 0;
}
RTN_COUNT* createRtnCount(RTN rtn, string progName){
    RTN_COUNT *routineCount = new RTN_COUNT;
    routineCount -> _name = RTN_Name(rtn);
    routineCount -> _image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
    routineCount -> _progName = progName;
    routineCount -> _address = RTN_Address(rtn);
    routineCount -> _icount = 0;
    routineCount -> _rtnCount = 0;

    return routineCount;
}
VOID printRtn(RTN_COUNT *rtnCount){
        string output = "Routine name: " + rtnCount -> _name + "Library: " + rtnCount -> _image;
        ofstream out("rtnList", ios::app);
        out << rtnCount -> _address << " Routine name: " << left << setw(25) << rtnCount -> _name << endl;
        out.close();
}

VOID traceRoutine(RTN rtn, VOID *v){
    char *argument = (char*)v;
    string progName(argument);

    RTN_COUNT *routineCount = new RTN_COUNT;
    routineCount -> _name = RTN_Name(rtn);
    routineCount -> _image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
    routineCount -> _progName = progName;
    routineCount -> _address = RTN_Address(rtn);
    routineCount -> _rtn = rtn;
    routineCount -> _icount = 0;
    routineCount -> _rtnCount = 0;
    routineCount -> _next = RtnList;
    RtnList = routineCount;
   
    RTN_Open(rtn);
    routineCount -> insHead = RTN_InsHead(rtn); 
    routineCount -> insTail = RTN_InsTail(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)printRtn, IARG_PTR, routineCount, IARG_END);
    RTN_Close(rtn);
}


VOID traceInstruction(INS ins, VOID *v){
    char *argument = (char*)v;
    string progName(argument);
    if(INS_IsCall(ins)){
        string OpIns = INS_Disassemble(ins);
        string address = OpIns.erase(0,5);
        const char *addr = address.c_str();
        ADDRINT opAddress = strtol(addr, NULL, 16);

        RTN rtn = INS_Rtn(ins);
        if(RTN_Valid(rtn)){
            cout << INS_Address(ins) << "  ";
            cout << RTN_Name(rtn) << " : ";
            cout << "call " << RTN_FindNameByAddress(opAddress) << "(" << opAddress << ")"  << endl;
        }
    }
    else{
        RTN rtn = INS_Rtn(ins);
        if(RTN_Valid(rtn)){
            cout << INS_Address(ins) << "  ";
            cout << RTN_Name(rtn) << " : ";
            cout << INS_Disassemble(ins) << endl;
        }
    }
}

int record = 0;
VOID traceGlobalTextIns(INS ins, VOID *v){
    if(INS_IsCall(ins) || INS_IsRet(ins)){
        string OpIns = INS_Disassemble(ins);
        ofstream out("callPath", ios::app);
        RTN rtn = INS_Rtn(ins);
        if(RTN_Valid(rtn)){
            if(INS_IsCall(ins)){
                char *argument = (char*)v; 
                string progName(argument);

                Node node;
                RTN_COUNT *routineCount = createRtnCount(rtn, progName);
                string address = OpIns.erase(0,5);
                const char *addr = address.c_str();
                ADDRINT opAddress = strtol(addr, NULL, 16);
                if(isTargetFunction(routineCount)){
                    record = 1;
                    cout << "T " << RTN_Name(rtn) << ":call " << RTN_FindNameByAddress(opAddress) << endl;
                }
                else if(record == 1){
                    cout << "N " <<  RTN_Name(rtn) << ":call " << RTN_FindNameByAddress(opAddress) << endl;
                }
            }
            else{
                if(record == 1){
                cout << "return " << RTN_Name(INS_Rtn(ins)) << endl;
                }
            }
        }
    }
}

VOID Fini(INT32 code, VOID *v){
    OutFile.setf(ios::showbase);
    cout << "finished\n" << endl;
    OutFile << "Pin finished." << endl;
    OutFile.close();
}


INT32 Usage(){
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc, char * argv[]){
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();

    INS_AddInstrumentFunction(traceGlobalTextIns, 0);
//    RTN_AddInstrumentFunction(traceRoutine, argv[8]);
//    INS_AddInstrumentFunction(traceInstruction, argv[8]);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();
    
    return 0;
}
