#define NOBITA_LINUX_IMPL
#include "nobita-linux.h"

void build(Nobita_Build *b) {
  Nobita_Exe *hello = Nobita_Build_Add_Exe(b, "hello");
  Nobita_Target_Set_Build_Tools(hello, "cc", NULL);
  Nobita_Target_Set_Build_Tool_Options(hello, "-c", "-o", NULL);
  Nobita_Target_Add_Sources(hello, "test-src/main.c", NULL);
}
