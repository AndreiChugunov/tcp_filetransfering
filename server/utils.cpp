//
// Created by 809279 on 10.11.2017.
//

#include "utils.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ftr {

    std::vector<std::string> parseString(char str[], char delim[]) {
        std::vector<std::string> vec;
        char *tok;
        tok = strtok(str, delim);
        while (tok != NULL) {
            std::string tokenString(tok);
            vec.push_back(tokenString);
            tok = strtok(NULL, delim);
        }
        return vec;
    }

    bool is_number(const std::string& s) {
        std::string::const_iterator it = s.begin();
        while (it != s.end() && std::isdigit(*it)) ++it;
        return !s.empty() && it == s.end();
    }
}