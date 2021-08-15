#ifndef __SHELL_SHELL_H
#define __SHELL_SHELL_H

#include "stdint.h"
#include "../fs/fs.h"

void print_prompt(void);
void readline(char* buf,int32_t count);
void my_shell(void);
int32_t cmd_parse(char* cmd_str,char** argv,char token);

#endif
