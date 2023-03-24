#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

char **readline_tok();
void free_tok(char **toks);
int toklen(char **toks);
