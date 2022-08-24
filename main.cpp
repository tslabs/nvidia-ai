
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <stdint.h>
using namespace std;

#include "gst.h"

OPTIONS options;

extern "C"
{
  extern char *optarg;
  int getopt(int nargc, char *const nargv[], const char *ostr);
}

int main(int argc, char *argv[])
{
  int c;
  while ((c = getopt(argc, argv, "c:f:j:g:v:os:m:i:p:d:")) != -1)
  {
    switch (c)
    {
      case 'f':
        options.src_path = optarg;
        break;
    }
  }

  // if (options.src_type == -1)
  // {
    // printf("Ooops!\n");
    // return 1;
  // }

  // ml_init();
  gst_my();

  return 0;
}
