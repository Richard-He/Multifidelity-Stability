#include <iostream>
#include <fstream>
#include <string>
using namespace std;

struct Token {
    string name;
    size_t lineNum, offset;
    Token(string s, size_t l, size_t o): name(s), lineNum(l), offset(o){}
};

class Tokenizer{
public:
    Token getToken();
    int getlineNum();
    int getoffset();
    bool nextToken();
    ifstream file;
    size_t lineNum, offset;
    string line;

    void closefile();
    void openfile(string input);
    Tokenizer();
    void reset();
};