#define NOBITA_IMPL
#include "nobita.h"

void build(Nobita_Build *b) {
  Nobita_Try_Rebuild(b, __FILE__);

  Nobita_Exe *hello = Nobita_Build_Add_Exe(b, "hello");
  Nobita_Target_Set_Build_Tool(hello, NOBITA_BT_MSVC);
  Nobita_Target_Add_Sources(hello, "test-src\\*.c", NULL);

  Nobita_Shared_Lib *a = Nobita_Build_Add_Shared_Lib(b, "a");
  Nobita_Target_Set_Build_Tool(a, NOBITA_BT_MSVC);
  Nobita_Target_Add_Sources(a, "test-src\\test-lib.c", NULL);
  Nobita_Target_Add_Headers(a, "3rd_party", "openssl\\sha.h", NULL);
  Nobita_Target_Add_Deps(a, hello, NULL);

  Nobita_Static_Lib *bb = Nobita_Build_Add_Static_Lib(b, "b");
  Nobita_Target_Set_Build_Tool(bb, NOBITA_BT_MSVC);
  Nobita_Target_Add_Sources(bb, "test-src\\test-lib.c", NULL);
  Nobita_Target_Add_Deps(bb, hello, NULL);

  Nobita_CMD *test3 = Nobita_Build_Add_CMD(b, "test");
  Nobita_CMD_Add_Args(test3, "echo", "test-src\\*.c", NULL);
  Nobita_Target_Add_Fmt_Arg(test3, NOBITA_T_CUSTOM_CMD, "%s",
                            "Hello, Croisen!");
}
