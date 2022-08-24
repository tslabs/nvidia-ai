
#pragma once
#include <string.h>

enum
{
  SRC_CAM,
  SRC_FILE
};

typedef struct
{
  string src_path;
} OPTIONS;

extern OPTIONS options;

int gst_my();
