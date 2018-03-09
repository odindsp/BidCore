#ifndef GETLOCATION_H_
#define GETLOCATION_H_

#include <stdint.h>
#include <string>
using std::string;

uint64_t openIPB(char *fname);
void closeIPB(uint64_t addr);
int getRegionCode(uint64_t addr, string &IPstr);

#endif
