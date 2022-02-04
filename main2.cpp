#include <fstream>
#include <iostream>
#include <vector>
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <map>
#include <iomanip>
#include <set>
#include "tokenizer.h"
#include <numeric>


// Do we debug or not ?
//#define DEBUG

// Some useful values for the boundary

#define MAX_IM_VAL 9999
#define MAX_SYMLEN 16
#define MAX_DEFCAP 16
#define MAX_USECAP 16
#define MAX_OPCODE 9
#define MAX_NUM ((1<<30)-1)
#define MEM_LIM 512

// Some parse error codes
#define NUM_EXP 0
#define SYM_EXP 1
#define ADDR_EXP 2
#define SYM_LONG 3
#define TOO_MANY_DEF 4
#define TOO_MANY_USE 5
#define TOO_MANY_INSTR 6

// Some error codes
#define ABSADDR_EX 0
#define READDR_EX 1
#define EXADDR_EX 2
#define SYM_UNDEFINE 3
#define ILL_I 5
#define ILL_OP 6
#define MUL_DEF 4

// Some Warning codes
#define DEF_ADDR_EX 0
#define SYM_UNUSED_MODULE 1
#define SYM_UNUSED 2

using namespace std;

// Helpful Functions to check string type
static inline bool is_not_alnum(char c)
{
    return !(isalnum(c));
}
static inline bool is_not_alpha(char c)
{
    return !(isalpha(c));
}
static inline bool is_not_digit(char c)
{
    return !(isdigit(c));
}
bool isalnum_s(const std::string &str)
{
    return find_if(str.begin(), str.end(), is_not_alnum) == str.end();
}
bool isalpha_s(const std::string &str)
{
    return find_if(str.begin(), str.end(), is_not_alpha) == str.end();
}
bool isdigit_s(const std::string &str)
{
    return find_if(str.begin(), str.end(), is_not_digit) == str.end();
}

//Variables at global view

map<string, int> Symtable; // Global Symbol Table

map<string, bool> Usetable; // Global used symbol table

map<string, bool> multideftable; // Multi-definition table of Symbol

map<string, int> sym2module;

//Variables at local view
map<string, int> local_Symtable; // deflist at each module

map<string, bool> local_Usetable; // uselist at each module and if they are used.

map<int, string> local_uselist;

Tokenizer tkz;

int total_instr = 0;
int total_module_cnt =0;
int total_offset =0;
// Init functions, clean up everything except Symtable
void reset_Usetable();
void Initialize(){
    tkz.reset();
    total_instr = 0;
    total_module_cnt =0;
    total_offset =0;
    local_Usetable.clear();
    local_Symtable.clear();
    reset_Usetable();
    local_uselist.clear();
}

// Some functions helping us debug
void printable_int(map<string, int>& symbolTable ,string name){
    printf("Debugging, printing the table of %s\n", name.c_str());
    map<string, int>::iterator it;

    for (it = symbolTable.begin(); it != symbolTable.end(); it++)
    {
        std::cout << it->first    // string (key)
                  << ':'
                  << it->second   // string's value
                  << std::endl;
    }
}

void printable_bool(map<string, bool > symbolTable, string name){
    printf("Debugging, printing the table of %s", name.c_str());
    map<string, bool>::iterator it;

    for (it = symbolTable.begin(); it != symbolTable.end(); it++)
    {
        std::cout << it->first    // string (key)
                  << ':'
                  << it->second   // string's value
                  << std::endl;
    }
}





// ParseError Wrapper
void parseerror(int errcode, int lineNum, int offset) {
    char const * errstr[] = {
            "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
            "SYM_EXPECTED", // Symbol Expected
            "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
            "SYM_TOO_LONG", // Symbol Name is too long
            "TOO_MANY_DEF_IN_MODULE", // > 16
            "TOO_MANY_USE_IN_MODULE", // > 16
            "TOO_MANY_INSTR",
    };
    printf("Parse Error line %d offset %d: %s\n", lineNum, offset, errstr[errcode]);
    abort();
}

// ErrorMessage Wrapper

void errormessage(int errcode, string name = ""){
    char const * errstr[] = {
            "Error: Absolute address exceeds machine size; zero used\n",
            "Error: Relative address exceeds module size; zero used\n",
            "Error: External address exceeds length of uselist; treated as immediate\n",
            "Error: %s is not defined; zero used\n",
            "Error: This variable is multiple times defined; first value used\n",
            "Error: Illegal immediate value; treated as 9999\n",
            "Error: Illegal opcode; treated as 9999\n",
    };
    if(errcode!= 3)
        printf(errstr[errcode]);
    else
        printf(errstr[errcode], name.c_str());
}

// Warning Wrapper

void warningmessage(int errcode, int module_num, string sym, int addr = 0, int module_size = 0){
    char const * errstr[] = {
            "Warning: Module %d: %s too big %d (max=%d) assume zero relative\n",
            "Warning: Module %d: %s appeared in the uselist but was not actually used\n",
            "Warning: Module %d: %s was defined but never used\n",
    };
    if(errcode!=0)
        printf(errstr[errcode], module_num, sym.c_str());
    else
        printf(errstr[errcode], module_num, sym.c_str(), addr, module_size);
}

// Check parseerror code
bool is_num(string num){
    if (!isdigit_s(num))
        return false ;
    return stoi(num) <= MAX_NUM;
}

bool is_sym(const string s){
    if(!isalpha(s[0])){
        return false ;
    }
    return isalnum_s(s);
}

bool is_addr(const string s){
    if (s.length()>1)
        return false ;
    return !(s != "A" && s != "E" && s != "I" && s != "R");
}

bool is_sym_too_long(const string s){
    return s.length() > MAX_SYMLEN;
}

bool is_too_many_def_in_module(){ // Check if the local symbol table is too full
    return local_Symtable.size() > MAX_DEFCAP;
}

bool is_too_many_use_in_module(){
    return local_Usetable.size() > MAX_USECAP;
}

bool is_too_many_instr(){
    return total_instr > MEM_LIM;
}

// Error message decider

bool is_abs_addr_exceed(int addr){
    return addr > MEM_LIM;
}

bool is_rel_addr_exceed(int readdr, int modulesize){
    return readdr > modulesize;
}

bool is_ext_addr_exceed(int extaddr, int uselistsize){
    return extaddr > uselistsize;
}

bool is_symbol_defined(const string symbol){
    return !(Symtable.find(symbol) == Symtable.end());
}

bool is_multi_defined(const string sym){
    return multideftable[sym];
}

bool is_ill_im_val(int val){
    return val>MAX_IM_VAL;
}

bool is_ill_op_code(int opcode){
    return opcode>MAX_OPCODE;
}

// Warnings
void check_oversize_def_in_module(int modulesize) {
    map<string, int>::iterator it;
    for (it = local_Symtable.begin(); it != local_Symtable.end(); it++) {
        const string sym = it->first;
        int absaddr = it->second;
        if (absaddr> modulesize) {
            warningmessage(DEF_ADDR_EX, total_module_cnt, sym, absaddr, modulesize-1);
            it->second = 0;
            Symtable[sym] = total_offset;
        }
    }
}

void check_unused_in_module(int id){
    map<string, bool>::iterator it;
    for (it = local_Usetable.begin(); it != local_Usetable.end(); it++) {
        if (! it->second)
            warningmessage(SYM_UNUSED_MODULE, id, it->first);
    }
#ifdef DEBUG
    printable_bool(local_Usetable,"local_Usetable");
#else
#endif
}

void check_define_unused(){
    map<string, bool>::iterator it;
    for (it = Usetable.begin(); it != Usetable.end(); it++) {
        if(! it->second)
            warningmessage(SYM_UNUSED, sym2module[it->first], it->first);
    }
#ifdef DEBUG
    printable_bool(Usetable,"Usetable");
#else
#endif
}

// Read Functions
int readInt(){// Try to read int from the file and return it
    Token tk = tkz.getToken();
    int lineNum = tk.lineNum;
    int offset = tk.offset;
    string num = tk.name;
    if(!is_num(num))
        parseerror(NUM_EXP,lineNum,offset);
    return stoi(num);
}

string readSymbol(){// Read Symbol
    Token tk = tkz.getToken();
    int lineNum = tk.lineNum;
    int offset = tk.offset;
    string symbol = tk.name;
    if(!is_sym(symbol))
        parseerror(SYM_EXP,lineNum,offset);
    if(is_sym_too_long(symbol))
        parseerror(SYM_LONG,lineNum,offset);
    if(is_too_many_def_in_module())
        parseerror(TOO_MANY_DEF, lineNum, offset);
    if(is_too_many_use_in_module())
        parseerror(TOO_MANY_USE, lineNum, offset);
    return symbol;
}

string readIAER(){
    total_instr ++;
    Token tk = tkz.getToken();
    int lineNum = tk.lineNum;
    int offset = tk.offset;
    string addr = tk.name;
    if(!is_addr(addr))
        parseerror(ADDR_EXP,lineNum,offset);
    if(is_too_many_instr())
        parseerror(TOO_MANY_INSTR,lineNum,offset);
    return addr;
}

void createModule(){
    local_Symtable.clear();
    local_Usetable.clear();
    local_uselist.clear();
    total_module_cnt++;
    total_offset=total_instr;
}

void createSymbol(string sym, int addr){
    if(!is_symbol_defined(sym)) {// Not found, initialize
        local_Symtable.insert(pair<string, int>(sym, addr));
        local_Usetable.insert(pair<string, bool>(sym, false));
        Symtable.insert(pair<string, int>(sym, addr + total_instr));
        Usetable.insert(pair<string, bool>(sym, false));
        multideftable.insert(pair<string, bool>(sym, false));
        sym2module.insert(pair<string, int>(sym, total_module_cnt));
    }
    else{
        multideftable[sym] = true; // Do nothing, using the first value
    }
}

void Update_uselist(const string sym, int ind){
    local_uselist.insert(pair<int,string>(ind, sym));
    local_Usetable.insert(pair<string, bool>(sym, false));
}

void Update_usetable(const string sym){
    Usetable[sym] = true;
    local_Usetable[sym] = true;
}

void printe(int absid, int val){
    cout << std::setfill('0') << std::setw(3) << absid;
    cout << ": "<<std::setfill('0') << std::setw(4) <<  val<<" ";
}

void PrintMap(const string instr, int op, int uselistsize, int modulesize){
    //First check opcode
    int opcode = op/1000;
    int oprand = op%1000;
    if(instr == "I"){
        if(is_ill_im_val(op)){
            printe(total_instr, 9999);
            errormessage(ILL_I);
        }
        else{
            printe(total_instr, op);
            cout<<endl;
        }
    }
    else if(is_ill_op_code(opcode)){
        printe(total_instr, 9999);
        errormessage(ILL_OP);
    }
    else if(instr == "E") {
        const string sym = local_uselist[oprand];
        if (is_ext_addr_exceed(oprand, uselistsize)) { // External Address overflow, treat as immediate
            printe(total_instr, op);
            errormessage(EXADDR_EX);
        } else if (!is_symbol_defined(sym)) {// External address not defined, treat as zero
            printe(total_instr, opcode * 1000);
            errormessage(SYM_UNDEFINE, sym);
            Update_usetable(sym);
        } else {
            int offset = Symtable[sym];
            printe(total_instr, opcode * 1000 + offset);
            cout << endl;
            Update_usetable(sym);
        }
    }
    else if(instr=="A"){
        if(is_abs_addr_exceed(oprand)) {
            printe(total_instr, opcode*1000); // Use absolute value zero
            errormessage(ABSADDR_EX);
        }
        else {
            printe(total_instr, op);
            cout<<endl;
        }
    }
    else if(instr == "R"){
        if(is_rel_addr_exceed(oprand, modulesize)){
           int newoprand = total_offset;
           printe(total_instr, opcode*1000+newoprand);
           errormessage(READDR_EX);
        }
        else{
            int newoprand = total_offset+oprand;
            printe(total_instr, opcode*1000+newoprand);
            cout<<endl;
        }
    }

}

void reset_Usetable(){
    map<string, bool>::iterator it;
    for (it = Usetable.begin(); it != Usetable.end(); it++) {
        it->second = false;
    }
}


void print_Symtable() {
    cout<<"Symbol Table"<<endl;
    map<string, int>::iterator it;
    for (it = Symtable.begin(); it != Symtable.end(); it++) {
        const string sym = it->first;
        cout<<sym<<"="<<it->second<<"  ";
        if(is_multi_defined(sym))
            errormessage(MUL_DEF);
        cout<<endl;
    }
    cout<<endl;
}

void checkdefcount(int defcount){
    if(defcount > MAX_DEFCAP){
        parseerror(TOO_MANY_DEF,tkz.lineNum,1);
    }
}
void checkusecount(int usecount){
    if(usecount > MAX_USECAP){
        parseerror(TOO_MANY_USE,tkz.lineNum,1);
    }
}
void checkinstrcount(int instrcount){
    if(instrcount > MEM_LIM){
        parseerror(TOO_MANY_INSTR,tkz.lineNum,1);
    }
}
void Pass1(string filename){
    tkz.openfile(filename);
    while(tkz.nextToken()){
        createModule();
        int defcount = readInt();
        checkdefcount(defcount);
        for (int i=0;i<defcount;i++){
            string sym = readSymbol();
            int val = readInt();
            createSymbol(sym, val);
        }
        int usecount = readInt();
        checkusecount(usecount);
        for (int i=0; i<usecount;i++){
            string sym = readSymbol();
        }
        int instcount = readInt();
        checkinstrcount(instcount+total_instr);
        for (int i=0; i<instcount; i++){
            string addressmode = readIAER();
            int operand = readInt();
        }
        check_oversize_def_in_module(instcount);
    }// Finish parsing
    print_Symtable();
    tkz.closefile();
}

void Pass2(string filename){
    tkz.openfile(filename);
    cout<<"Memory Map"<<endl;
    while(tkz.nextToken()){
        createModule();
        int defcount = readInt();
        for (int i=0;i<defcount;i++){
            string sym = readSymbol();
            int val = readInt();
        }
        int usecount = readInt();
        for (int i=0; i<usecount;i++){
            string sym = readSymbol();
            Update_uselist(sym, i);
        }
        int instcount = readInt();
        for (int i=0; i<instcount; i++){
            string instr = readIAER();
            int op = readInt();
            PrintMap(instr,op,usecount,instcount);
        }
        check_unused_in_module(total_module_cnt);
    }
    cout<<endl;
    check_define_unused();
    tkz.closefile();
}

int main(int argc, char* argv[]) {
    string filename(argv[1]);
//    string filename("/Users/he/OSLab/Linker/lab1_assign/input-20");
    Pass1(filename);
    Initialize();
    Pass2(filename);
    return 0;
}