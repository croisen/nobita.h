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

#define NOBITA_LINUX_IMPL
#ifdef NOBITA_LINUX_IMPL

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

enum nobita_target_type {
  NOBITA_EXECUTABLE,
  NOBITA_SHARED_LIB,
  NOBITA_STATIC_LIB,
};

struct nobita_target {
  char *name;
  bool built;
  enum nobita_target_type target_type;

  char *cc;
  char *ar;
  char *cc_to_obj_opt;
  char *cc_out_file_opt;
  char *ar_create_opts;
  char *prefix;

  size_t cflags_used;
  size_t cflags_size;
  char **cflags;

  size_t sources_used;
  size_t sources_size;
  char **sources;

  size_t ldflags_used;
  size_t ldflags_size;
  char **ldflags;

  size_t full_cmd_used;
  size_t full_cmd_size;
  char **full_cmd;

  size_t dep_libs_used;
  size_t dep_libs_size;
  struct nobita_target **dep_libs;

  size_t dep_exes_used;
  size_t dep_exes_size;
  struct nobita_target **dep_exes;
};

struct nobita_build {
  size_t dep_libs_used;
  size_t dep_libs_size;
  struct nobita_target **dep_libs;

  size_t dep_exes_used;
  size_t dep_exes_size;
  struct nobita_target **dep_exes;
};

#define vector_init(ptr, name)                                                 \
  do {                                                                         \
    (ptr)->name##_size = 8;                                                    \
    (ptr)->name##_used = 0;                                                    \
    (ptr)->name = calloc(8, sizeof(*(ptr)->name));                             \
    assert((ptr)->name != NULL && "Buy more ram lol");                         \
  } while (false)

#define vector_free(ptr, name)                                                 \
  do {                                                                         \
    free((ptr)->name);                                                         \
    (ptr)->name##_size = 0;                                                    \
    (ptr)->name##_used = 0;                                                    \
    (ptr)->name = NULL;                                                        \
  } while (false)

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
  } while (false)

#define vector_append_vector(dest, dest_name, src, src_name)                   \
  do {                                                                         \
    for (size_t nobita_iter_##src_name = 0;                                    \
         nobita_iter_##src_name < (src)->src_name##_used;                      \
         nobita_iter_##src_name++)                                             \
      vector_append(dest, dest_name, (src)->src_name[nobita_iter_##src_name]); \
  } while (false)

static struct nobita_target *nobita_build_add_target(Nobita_Build *b,
                                                     const char *name) {
  struct nobita_target *t = malloc(sizeof(*t));
  assert(t != NULL && "Buy more ram lol");
  t->name = malloc((strlen(name) + 1) * sizeof(char));
  assert(t->name != NULL && "Buy more ram lol");
  strcpy(t->name, name);

  t->built = false;
  t->ar = NULL;
  t->cc = NULL;
  t->cc_to_obj_opt = NULL;
  t->cc_out_file_opt = NULL;
  t->ar_create_opts = NULL;
  vector_init(t, cflags);
  vector_init(t, sources);
  vector_init(t, ldflags);
  vector_init(t, full_cmd);
  return t;
}

Nobita_Exe *Nobita_Build_Add_Exe(Nobita_Build *b, const char *name) {
  Nobita_Exe *e = nobita_build_add_target(b, name);
  e->target_type = NOBITA_EXECUTABLE;
  vector_append(b, dep_exes, e);
  return e;
}

Nobita_Shared_Lib *Nobita_Build_Add_Shared_Lib(Nobita_Build *b,
                                               const char *name) {
  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  l->target_type = NOBITA_SHARED_LIB;
  vector_append(b, dep_libs, l);
  return l;
}

Nobita_Static_Lib *Nobita_Build_Add_Static_Lib(Nobita_Build *b,
                                               const char *name) {
  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  l->target_type = NOBITA_STATIC_LIB;
  vector_append(b, dep_libs, l);
  return l;
}

void Nobita_Target_Set_Build_Tools(struct nobita_target *t, const char *cc,
                                   const char *ar) {
  assert(cc != NULL && "Please do not set the compiler arg to NULL");
  t->cc = malloc((strlen(cc) + 1) * sizeof(char));
  assert(t->cc != NULL && "Buy more ram lol");
  strcpy(t->cc, cc);

  if ((t->target_type == NOBITA_SHARED_LIB ||
       t->target_type == NOBITA_STATIC_LIB)) {
    if (ar == NULL) {
      assert(false && "Please do not set the archiver to NULL if you're "
                      "building a library");
    } else {
      t->ar = malloc((strlen(ar) + 1) * sizeof(char));
      assert(t->ar == NULL && "Buy more ram lol");
      strcpy(t->ar, ar);
    }
  }
}
void Nobita_Target_Set_Build_Tool_Options(struct nobita_target *t,
                                          const char *cc_to_obj_opt,
                                          const char *cc_out_file_opt,
                                          const char *ar_create_opts) {
  assert((cc_to_obj_opt != NULL || cc_out_file_opt != NULL) &&
         "Please do not set any of the compiler options to NULL");
  t->cc_to_obj_opt = malloc((strlen(cc_to_obj_opt) + 1) * sizeof(char));
  assert(t->cc_to_obj_opt != NULL && "Buy more ram lol");
  strcpy(t->cc_to_obj_opt, cc_to_obj_opt);

  t->cc_out_file_opt = malloc((strlen(cc_out_file_opt) + 1) * sizeof(char));
  assert(t->cc_out_file_opt != NULL && "Buy more ram lol");
  strcpy(t->cc_out_file_opt, cc_out_file_opt);

  if ((t->target_type == NOBITA_SHARED_LIB ||
       t->target_type == NOBITA_STATIC_LIB)) {
    if (ar_create_opts == NULL) {
      assert(false && "Please do not set ar_create_opts to NULL if you're "
                      "building a library");
    } else {
      t->ar_create_opts = malloc((strlen(ar_create_opts) + 1) * sizeof(char));
      assert(t->ar_create_opts == NULL && "Buy more ram lol");
      strcpy(t->ar_create_opts, ar_create_opts);
    }
  }
}

void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix) {
  assert(prefix != NULL && "Please do not set prefix to NULL");
  assert(*prefix == '/' && "Prefix path should be an absolute path");

  t->prefix = malloc((strlen(prefix) + 1) * sizeof(char));
  assert(t->prefix != NULL && "Buy more ram lol");
  strcpy(t->prefix, prefix);
}

void Nobita_Target_Add_Cflags(struct nobita_target *t, ...) {
  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *s = malloc((strlen(arg) + 1) * sizeof(char));
    assert(s != NULL && "Buy more ram lol");
    strcpy(s, arg);
    vector_append(t, cflags, s);

    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_Sources(struct nobita_target *t, ...) {
  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *s = malloc((strlen(arg) + 1) * sizeof(char));
    assert(s != NULL && "Buy more ram lol");
    strcpy(s, arg);
    vector_append(t, sources, s);

    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_LDflags(struct nobita_target *t, ...) {
  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *s = malloc((strlen(arg) + 1) * sizeof(char));
    assert(s != NULL && "Buy more ram lol");
    strcpy(s, arg);
    vector_append(t, ldflags, s);

    arg = va_arg(va, char *);
  }

  va_end(va);
}

static void fork_exec(char **cmd) {
  pid_t id = fork();
  if (id == 0) {
    execvp(*cmd, cmd);
    exit(EXIT_FAILURE);
  } else {
    size_t full_cmd_len = 1;
    char *full_cmd = NULL;
    char **cmd2 = cmd;
    while (*cmd2 != NULL) {
      full_cmd_len += strlen(*cmd2) + 1;
      cmd2++;
    }
    full_cmd = calloc(full_cmd_len, sizeof(char));
    while (*cmd) {
      strcat(full_cmd, *cmd);
      strcat(full_cmd, " ");
      cmd++;
    }

    printf("%s\n", full_cmd);
    free(full_cmd);
    waitpid(id, NULL, 0);
  }
}

static void build_exe(Nobita_Exe *e) {
  vector_append(e, full_cmd, e->cc);
  vector_append_vector(e, full_cmd, e, cflags);
  vector_append(e, full_cmd, e->cc_out_file_opt);
  vector_append(e, full_cmd, e->name);
  vector_append_vector(e, full_cmd, e, sources);
  vector_append_vector(e, full_cmd, e, ldflags);
  vector_append(e, full_cmd, NULL);

  fork_exec(e->full_cmd);
}

int main(void) {
  struct nobita_build b;
  vector_init(&b, dep_libs);
  vector_init(&b, dep_exes);

  build(&b);

  for (size_t i = 0; i < b.dep_exes_used; i++)
    build_exe(b.dep_exes[i]);

  for (size_t i = 0; i < b.dep_libs_used + b.dep_exes_used; i++) {
    struct nobita_target *t;
    if (i < b.dep_libs_used)
      t = b.dep_libs[i];
    else
      t = b.dep_exes[i - b.dep_libs_used];

    free(t->name);
    free(t->cc);
    free(t->ar);
    free(t->cc_to_obj_opt);
    free(t->cc_out_file_opt);
    free(t->ar_create_opts);

    for (size_t ii = 0; ii < t->cflags_used; ii++)
      free(t->cflags[ii]);

    for (size_t ii = 0; ii < t->sources_used; ii++)
      free(t->sources[ii]);

    for (size_t ii = 0; ii < t->ldflags_used; ii++)
      free(t->ldflags[ii]);

    vector_free(t, cflags);
    vector_free(t, sources);
    vector_free(t, ldflags);
    vector_free(t, full_cmd);
    free(t);
  }

  vector_free(&b, dep_libs);
  vector_free(&b, dep_exes);
}
#endif /* NOBITA_LINUX_IMPL */
