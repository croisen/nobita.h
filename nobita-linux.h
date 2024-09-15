#ifndef NOBITA_LINUX_H
#define NOBITA_LINUX_H

typedef struct nobita_build Nobita_Build;
typedef struct nobita_target Nobita_Exe;
typedef struct nobita_target Nobita_Shared_Lib;
typedef struct nobita_target Nobita_Static_Lib;

extern void build(Nobita_Build *b);

Nobita_Exe *Nobita_Build_Add_Exe(Nobita_Build *b, const char *name);
Nobita_Shared_Lib *Nobita_Build_Add_Shared_Lib(Nobita_Build *b,
                                               const char *name);
Nobita_Static_Lib *Nobita_Build_Add_Static_Lib(Nobita_Build *b,
                                               const char *name);

void Nobita_Target_Set_Build_Tools(struct nobita_target *t, const char *cc,
                                   const char *ar);
void Nobita_Target_Set_Build_Tool_Options(struct nobita_target *t,
                                          const char *cc_to_obj_opt,
                                          const char *cc_out_file_opt,
                                          const char *ar_create_opts);
void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix);

void Nobita_Target_Add_Cflags(struct nobita_target *t, ...);
void Nobita_Target_Add_Sources(struct nobita_target *t, ...);
void Nobita_Target_Add_LDflags(struct nobita_target *t, ...);

#endif /* NOBITA_LINUX_H */

// #define NOBITA_LINUX_IMPL
#ifdef NOBITA_LINUX_IMPL

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define vector_init(ptr, name)                                                 \
  do {                                                                         \
    (ptr)->name##_size = 8;                                                    \
    (ptr)->name##_used = 0;                                                    \
    (ptr)->name = calloc(8, sizeof(*(ptr)->name));                             \
    assert((ptr)->name != NULL && "Buy more ram lol");                         \
  } while (0)

#define vector_free(ptr, name)                                                 \
  do {                                                                         \
    free((ptr)->name);                                                         \
    (ptr)->name##_size = 0;                                                    \
    (ptr)->name##_used = 0;                                                    \
    (ptr)->name = NULL;                                                        \
  } while (0)

#define vector_append(ptr, name, item)                                         \
  do {                                                                         \
    if ((ptr)->name##_size <= (ptr)->name##_used + 1) {                        \
      void *data = calloc((ptr)->name##_size * 2, sizeof(*(ptr)->name));       \
      assert(data != NULL && "Buy more ram lol");                              \
      memcpy(data, (ptr)->name, (ptr)->name##_used * sizeof(*(ptr)->name));    \
      free((ptr)->name);                                                       \
      (ptr)->name = data;                                                      \
      (ptr)->name##_size *= 2;                                                 \
    }                                                                          \
                                                                               \
    (ptr)->name[(ptr)->name##_used] = item;                                    \
    (ptr)->name##_used += 1;                                                   \
  } while (0)

int main(void) { build(NULL); }

#endif /* NOBITA_LINUX_IMPL */
