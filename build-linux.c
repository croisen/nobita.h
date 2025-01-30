#define NOBITA_LINUX_IMPL
#include "nobita-linux.h"

void build(Nobita_Build *b) {
  Nobita_Try_Rebuild(b, __FILE__);

  Nobita_Exe *hello = Nobita_Build_Add_Exe(b, "hello");
  Nobita_Target_Set_Build_Tools(hello, "cc", NULL);
  Nobita_Target_Set_Build_Tool_Options(hello, "-c", "-o", NULL, NULL);
  Nobita_Target_Add_Sources(hello, "test-src/*.c", NULL);

  Nobita_CMD *test3 = Nobita_Add_CMD(b, "test");
  Nobita_CMD_Add_Args(test3, "echo", "test-src/*.c", NULL);
  Nobita_Target_Add_Fmt_Arg(test3, NOBITA_T_CUSTOM_CMD, "%s",
                            "Hello, Croisen!");
}
