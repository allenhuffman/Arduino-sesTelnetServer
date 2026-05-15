#ifndef SESTATPARSER_H
#define SESTATPARSER_H

#include <stdbool.h>
#include <stddef.h>     // for size_t
#include <stdint.h>

void    cmdMode(void);
uint8_t readCmdLine(char *cmdLine, size_t len);
bool    cmdModeCheck(char ch);

#endif /* SESTATPARSER_H */
