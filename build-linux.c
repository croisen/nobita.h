#define NOBITA_LINUX_IMPL
#include "nobita-linux.h"

void build(Nobita_Build *b) {
  Nobita_Try_Rebuild(b, __FILE__);

  Nobita_Exe *hello = Nobita_Build_Add_Exe(b, "hello");
  Nobita_Target_Set_Build_Tools(hello, "cc", NULL);
  Nobita_Target_Set_Build_Tool_Options(hello, "-c", "-o", NULL, NULL);
  Nobita_Target_Add_Sources(hello, "test-src/main.c", NULL);

  Nobita_Shared_Lib *test = Nobita_Build_Add_Shared_Lib(b, "test");
  Nobita_Target_Set_Build_Tools(test, "cc", "ar");
  Nobita_Target_Set_Build_Tool_Options(test, "-c", "-o", "-shared", "rcs");
  Nobita_Target_Add_Sources(test, "test-src/test-lib.c", NULL);
  Nobita_Target_Add_Deps(hello, test, NULL);

  Nobita_Shared_Lib *test2 = Nobita_Build_Add_Static_Lib(b, "test");
  Nobita_Target_Set_Build_Tools(test2, "cc", "ar");
  Nobita_Target_Set_Build_Tool_Options(test2, "-c", "-o", "-shared", "rcs");
  Nobita_Target_Add_Sources(test2, "test-src/test-lib.c", NULL);
  Nobita_Target_Add_Deps(hello, test2, NULL);
  Nobita_Target_Add_Headers(test2, "3rd_party", "openssl/sha.h", NULL);

  Nobita_CMD *test3 = Nobita_Add_CMD(b, "test");
  Nobita_CMD_Add_Args(test3, "echo", NULL);
  Nobita_Target_Add_Fmt_Arg(test3, NOBITA_T_CUSTOM_CMD, "**%s**\n",
                            "Hello, Croisen!");
}
