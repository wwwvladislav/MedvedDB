#pragma once
#include "mdv_def.h"


typedef struct
{
    mdv_descriptor rd;
    mdv_descriptor wd;
} mdv_pipe;


mdv_errno mdv_pipe_open(mdv_pipe *pipe);
void      mdv_pipe_close(mdv_pipe *pipe);
