#include <iostream>
#include <fstream>
#include "tokenizer.h"
#include <algorithm>
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#   define ASSERT(condition, message) do { } while (false)

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

Tokenizer::Tokenizer(){
    lineNum = 0;
    offset = 0;
}

void Tokenizer::openfile(string input) {
    file.open(input);
//    lineNum++;
//    getline(file,line);
}

int Tokenizer::getlineNum() {
    return lineNum;
}

int Tokenizer::getoffset() {
    return offset;
}

void Tokenizer::closefile(){
    file.close();
}

bool Tokenizer::nextToken() {// If there is still tokens, return true otherwise false And we put the offset to the next valid
    //character
//    while(line.at(offset) == ' '|| line.at(offset) =='\t'){
//        offset++;
//        while(offset >=line.length()){
//            if (getline(file, line)){
//                lineNum++;
//                offset = 0;
//            }
//            else
//                return false;
//        }
//    }
    while(offset >= line.length()){// If at the end, turn to the next line
        if (getline(file, line)) {
            line = rtrim(line);
            lineNum++;
            offset = 0;
        } else {
            return false;
        }

    }
    while(offset < line.length()){//
        char c = line.at(offset);// string.at() will be better than [] because it does sanity check
        if (c != '\t' && c != ' '){
            break;
        }
        else
            offset++;
    }
    return true;
}


Token Tokenizer::getToken() { // Get the next valid token
    int start=0;
    if(nextToken()){// Hit the first,  try to find the last position
        start = offset;
        ASSERT(string.at(offset)!=' '&&string.at(offset)!='\t', "offset"<<offset<<"line_no"<<lineNum);
        while (offset < line.length()) {
            if (line.at(offset) == ' ' || line.at(offset) == '\t') {
                break;
            }
            offset++;
        }
        Token token(line.substr(start, offset - start), lineNum, start+1);
        return token;
    }
//    cout<<"Wrong"<<lineNum<<" "<<offset+1<<endl;
    Token token("", lineNum, offset+1);
    return token;
}


void Tokenizer::reset(){
    lineNum = 0;
    offset = 0;
}





