//
// Created by heyihan on 2/4/22.
//

#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <ctype.h>
#include <algorithm>
#include <map>
#include <iomanip>
#include <set>
#include "tokenizer.h"
#include <numeric>
#include<fstream>
using namespace std;


int main() {
//    ifstream myfile;
//    myfile.open("/Users/he/OSLab/Linker/textt.txt");
//    string line ;
//    getline(myfile, line);
//    cout<<line<<endl;
    Tokenizer a;
    a.openfile("/Users/he/OSLab/Linker/textt.txt");
    cout<<a.nextToken();
    while(a.nextToken()) {
//        cout<<a.lineNum<<" "<<a.offset;
        Token token = a.getToken();
        cout << "Token_offset:" << token.offset << endl;
        cout << "Token_lineNum:" << token.lineNum << endl;
        cout << token.name << endl;
    }
    a.closefile();
    return 0;
}