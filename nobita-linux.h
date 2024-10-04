/**
 * Copyright © 2024 croisen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NOBITA_LINUX_H
#define NOBITA_LINUX_H

typedef struct nobita_build Nobita_Build;
typedef struct nobita_target Nobita_Exe;
typedef struct nobita_target Nobita_Shared_Lib;
typedef struct nobita_target Nobita_Static_Lib;

extern void build(Nobita_Build *b);

void Nobita_Try_Rebuild(Nobita_Build *b, const char *build_file);

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
                                          const char *cc_shared_lib_opt,
                                          const char *ar_create_opts);
void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix);

void Nobita_Target_Add_Cflags(struct nobita_target *t, ...);
void Nobita_Target_Add_Sources(struct nobita_target *t, ...);
void Nobita_Target_Add_LDflags(struct nobita_target *t, ...);
void Nobita_Target_Add_Deps(struct nobita_target *t, ...);

#endif /* NOBITA_LINUX_H */

#define NOBITA_LINUX_IMPL
#ifdef NOBITA_LINUX_IMPL

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libgen.h>
#include <unistd.h>

enum nobita_target_type {
  NOBITA_EXECUTABLE,
  NOBITA_SHARED_LIB,
  NOBITA_STATIC_LIB,
};

struct nobita_target {
  char *name;
  bool built;
  bool rebuilt;
  enum nobita_target_type target_type;

  char *cc;
  char *ar;
  char *cc_to_obj_opt;
  char *cc_out_file_opt;
  char *cc_shared_lib_opt;
  char *ar_create_opts;
  char *prefix;

  size_t cflags_used;
  size_t cflags_size;
  char **cflags;

  size_t sources_used;
  size_t sources_size;
  char **sources;

  size_t objects_used;
  size_t objects_size;
  char **objects;

  size_t ldflags_used;
  size_t ldflags_size;
  char **ldflags;

  size_t full_cmd_used;
  size_t full_cmd_size;
  char **full_cmd;

  size_t deps_used;
  size_t deps_size;
  // nobita_target - Well clangd keeps bugging me with suspicious
  // sizeof deref uses sooo
  void **deps;
};

struct nobita_build {
  size_t deps_used;
  size_t deps_size;
  // nobita_target - Well clangd keeps bugging me with suspicious
  // sizeof deref uses sooo
  void **deps;

  int argc;
  char **argv;
  bool was_self_rebuilt;
};

char *strdup(const char *);
static struct nobita_target *nobita_build_add_target(struct nobita_build *b,
                                                     const char *name);
static void nobita_fork_exec(char **cmd);
static char *nobita_dir_append(const char *prefix, ...);
static void nobita_mkdir_recursive(const char *path);
static bool nobita_is_a_newer(char *a, char *b);
static void nobita_build_target(struct nobita_target *t);

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

Nobita_Exe *Nobita_Build_Add_Exe(Nobita_Build *b, const char *name) {
  Nobita_Exe *e = nobita_build_add_target(b, name);
  e->target_type = NOBITA_EXECUTABLE;
  return e;
}

Nobita_Shared_Lib *Nobita_Build_Add_Shared_Lib(Nobita_Build *b,
                                               const char *name) {
  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  l->target_type = NOBITA_SHARED_LIB;
  return l;
}

Nobita_Static_Lib *Nobita_Build_Add_Static_Lib(Nobita_Build *b,
                                               const char *name) {
  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  l->target_type = NOBITA_STATIC_LIB;
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
      assert(t->ar != NULL && "Buy more ram lol");
      strcpy(t->ar, ar);
    }
  }
}

void Nobita_Target_Set_Build_Tool_Options(struct nobita_target *t,
                                          const char *cc_to_obj_opt,
                                          const char *cc_out_file_opt,
                                          const char *cc_shared_lib_opt,
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
    assert(ar_create_opts != NULL &&
           "Please do not set ar_create_opts to NULL if you're "
           "building a library");
    t->ar_create_opts = malloc((strlen(ar_create_opts) + 1) * sizeof(char));
    assert(t->ar_create_opts != NULL && "Buy more ram lol");
    strcpy(t->ar_create_opts, ar_create_opts);
  }

  if (t->target_type == NOBITA_SHARED_LIB) {
    assert(cc_shared_lib_opt != NULL && "The shared lib opt must not be NULL "
                                        "if you're building a shared library");
    t->cc_shared_lib_opt =
        malloc((strlen(cc_shared_lib_opt) + 1) * sizeof(char));
    assert(t->cc_shared_lib_opt != NULL && "Buy more ram lol");
    strcpy(t->cc_shared_lib_opt, cc_shared_lib_opt);
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
  char *cwd = getcwd(NULL, 0);
  assert(cwd != NULL && "Could not get current working directory");

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *s = malloc((strlen(arg) + 1) * sizeof(char));
    assert(s != NULL && "Buy more ram lol");
    strcpy(s, arg);
    char *o = malloc((strlen(arg) + 1) * sizeof(char));
    assert(o != NULL && "Buy more ram lol");
    strcpy(o, arg);

    char *s1 = nobita_dir_append(cwd, s, NULL);
    vector_append(t, sources, s1);

    char *i = o + strlen(o) - 1;
    size_t from_end = 0;
    while (i > o) {
      if (*i == '.') {
        memset(i + 1, 0, from_end * sizeof(char));
        i[1] = 'o';
        break;
      }

      i--;
      from_end++;
    }

    char *o1 = nobita_dir_append(cwd, "nobita-cache", o, NULL);
    char *o2 = strdup(o1);
    assert(o2 != NULL && "Buy more ram lol");

    vector_append(t, objects, o1);
    dirname(o2);
    nobita_mkdir_recursive(o2);
    free(o2);

    free(s);
    free(o);

    arg = va_arg(va, char *);
  }

  free(cwd);
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

void Nobita_Target_Add_Deps(struct nobita_target *t, ...) {
  va_list va;
  va_start(va, t);

  struct nobita_target *arg = va_arg(va, struct nobita_target *);
  while (arg != NULL) {
    vector_append(t, deps, arg);
    arg = va_arg(va, struct nobita_target *);
  }

  va_end(va);
}

void Nobita_Try_Rebuild(Nobita_Build *b, const char *build_file) {
  char *build_exe = *b->argv;
  char *nobita = __FILE__;

  if (nobita_is_a_newer(build_exe, (char *)build_file) &&
      nobita_is_a_newer(build_exe, nobita))
    return;

  struct nobita_target e;
  vector_init(&e, full_cmd);
#if defined(__clang__)
  vector_append(&e, full_cmd, "clang");
#elif defined(__GNUC__)
  vector_append(&e, full_cmd, "gcc");
#else
  vector_append(&e, full_cmd, "cc");
#endif

  vector_append(&e, full_cmd, "-Wall");
  vector_append(&e, full_cmd, "-Wextra");
  vector_append(&e, full_cmd, "-Wpedantic");
  vector_append(&e, full_cmd, "-fsanitize=address");
  vector_append(&e, full_cmd, "-O3");
  vector_append(&e, full_cmd, "-g");
  vector_append(&e, full_cmd, "-o");
  vector_append(&e, full_cmd, build_exe);
  vector_append(&e, full_cmd, (char *)build_file);
  vector_append(&e, full_cmd, NULL);

  nobita_fork_exec(e.full_cmd);
  e.full_cmd_used = 0;
  for (int i = 0; i < b->argc; i++)
    vector_append(&e, full_cmd, b->argv[i]);

  vector_append(&e, full_cmd, NULL);
  nobita_fork_exec(e.full_cmd);
  vector_free(&e, full_cmd);
  b->was_self_rebuilt = true;
}

static struct nobita_target *nobita_build_add_target(struct nobita_build *b,
                                                     const char *name) {
  struct nobita_target *t = malloc(sizeof(*t));
  assert(t != NULL && "Buy more ram lol");
  t->name = malloc((strlen(name) + 1) * sizeof(char));
  assert(t->name != NULL && "Buy more ram lol");
  strcpy(t->name, name);

  t->built = false;
  t->rebuilt = false;
  t->ar = NULL;
  t->cc = NULL;
  t->cc_to_obj_opt = NULL;
  t->cc_out_file_opt = NULL;
  t->cc_shared_lib_opt = NULL;
  t->ar_create_opts = NULL;
  vector_init(t, cflags);
  vector_init(t, sources);
  vector_init(t, objects);
  vector_init(t, ldflags);
  vector_init(t, full_cmd);
  vector_init(t, deps);

  vector_append(b, deps, t);
  return t;
}

static void nobita_fork_exec(char **cmd) {
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

static char *nobita_dir_append(const char *prefix, ...) {
  size_t len = 2 + strlen(prefix);
  va_list va;
  va_start(va, prefix);

  char *arg = va_arg(va, char *);
  while (true) {
    len += strlen(arg);
    arg = va_arg(va, char *);
    if (arg != NULL)
      len += 1;
    else
      break;
  }

  va_end(va);

  char *append = malloc(len * sizeof(char));
  assert(append != NULL && "Cannot allocate memory for str concat");
  strcpy(append, prefix);
  strcat(append, "/");
  va_start(va, prefix);

  arg = va_arg(va, char *);
  while (true) {
    strcat(append, arg);
    arg = va_arg(va, char *);
    if (arg != NULL)
      strcat(append, "/");
    else
      break;
  }

  va_end(va);
  return append;
}

static void nobita_mkdir_recursive(const char *path) {
  char *p2 = strdup(path);
  assert(p2 != NULL && "Buy more ram lol");
  size_t len = strlen(p2);

  for (size_t i = 1; i < len; i++) {
    if (p2[i] == '/') {
      p2[i] = 0;
      mkdir(p2, 0777);
      p2[i] = '/';
    }
  }

  mkdir(p2, 0777);
  free(p2);
}

static bool nobita_is_a_newer(char *a, char *b) {
  struct stat s_a, s_b;
  memset(&s_a, 0, sizeof(s_a));
  memset(&s_b, 0, sizeof(s_b));
  stat(a, &s_a);
  stat(b, &s_b);

  return (s_a.st_mtime > s_b.st_mtime);
}

static void nobita_build_target(struct nobita_target *t) {
  if (t->built)
    return;

  bool rebuild = false;
  for (size_t i = 0; i < t->deps_used; i++) {
    nobita_build_target(t->deps[i]);
    rebuild = (!rebuild) ? ((struct nobita_target *)t->deps[i])->rebuilt : true;
  }

  for (size_t i = 0; i < t->objects_used; i++) {
    if (!nobita_is_a_newer(t->objects[i], t->sources[i]) || rebuild) {
      vector_append(t, full_cmd, t->cc);
      vector_append_vector(t, full_cmd, t, cflags);
      vector_append(t, full_cmd, t->cc_out_file_opt);
      vector_append(t, full_cmd, t->objects[i]);
      vector_append(t, full_cmd, t->cc_to_obj_opt);
      vector_append(t, full_cmd, t->sources[i]);
      vector_append(t, full_cmd, NULL);

      nobita_fork_exec(t->full_cmd);
      t->full_cmd_used = 0;
      rebuild = true;
    }
  }

  char *name = malloc((strlen(t->name) + 7) * sizeof(char));
  char *cwd = getcwd(NULL, 0);
  assert(name != NULL && "Buy more ram lol");
  assert(cwd != NULL && "Could not get current working directory");

  char *prefix = nobita_dir_append(cwd, "nobita-build", NULL);
  char *bin = nobita_dir_append(prefix, "bin", NULL);
  char *lib = nobita_dir_append(prefix, "lib", NULL);
  char *include = nobita_dir_append(prefix, "include", NULL);
  char *output = NULL;

  mkdir(prefix, 0777);
  mkdir(bin, 0777);
  mkdir(lib, 0777);
  mkdir(include, 0777);

  switch (t->target_type) {
  case NOBITA_EXECUTABLE: {
    strcpy(name, t->name);
    output = nobita_dir_append(bin, name, NULL);
    break;
  }
  case NOBITA_SHARED_LIB: {
    strcpy(name, "lib");
    strcat(name, t->name);
    strcat(name, ".so");
    output = nobita_dir_append(lib, name, NULL);
    break;
  }
  case NOBITA_STATIC_LIB: {
    strcpy(name, "lib");
    strcat(name, t->name);
    strcat(name, ".a");
    output = nobita_dir_append(lib, name, NULL);
    break;
  }
  }

  if (!rebuild)
    for (size_t i = 0; i < t->objects_used; i++)
      rebuild = (!rebuild) ? nobita_is_a_newer(t->objects[i], output) : true;

  if (!rebuild)
    goto end;

  switch (t->target_type) {
  case NOBITA_EXECUTABLE: {
    vector_append(t, full_cmd, t->cc);
    vector_append_vector(t, full_cmd, t, cflags);
    vector_append(t, full_cmd, t->cc_out_file_opt);
    break;
  }
  case NOBITA_SHARED_LIB: {
    vector_append(t, full_cmd, t->cc);
    vector_append_vector(t, full_cmd, t, cflags);
    vector_append(t, full_cmd, t->cc_shared_lib_opt);
    vector_append(t, full_cmd, t->cc_out_file_opt);
    break;
  }
  case NOBITA_STATIC_LIB: {
    vector_append(t, full_cmd, t->ar);
    vector_append(t, full_cmd, t->ar_create_opts);
    break;
  }
  }

  vector_append(t, full_cmd, output);
  vector_append_vector(t, full_cmd, t, objects);

  if (t->target_type != NOBITA_STATIC_LIB)
    vector_append_vector(t, full_cmd, t, ldflags);

  vector_append(t, full_cmd, NULL);
  nobita_fork_exec(t->full_cmd);
  t->rebuilt = true;

end:
  t->built = true;
  free(name);
  free(output);
  free(cwd);
  free(prefix);
  free(bin);
  free(lib);
  free(include);
}

int main(int argc, char **argv) {
  struct nobita_build b;
  vector_init(&b, deps);
  b.argc = argc;
  b.argv = argv;
  b.was_self_rebuilt = false;

  build(&b);

  if (!b.was_self_rebuilt)
    for (size_t i = 0; i < b.deps_used; i++)
      nobita_build_target(b.deps[i]);

  for (size_t i = 0; i < b.deps_used; i++) {
    struct nobita_target *t = b.deps[i];

    free(t->name);
    free(t->cc);
    free(t->ar);
    free(t->cc_to_obj_opt);
    free(t->cc_out_file_opt);
    free(t->cc_shared_lib_opt);
    free(t->ar_create_opts);

    for (size_t ii = 0; ii < t->cflags_used; ii++)
      free(t->cflags[ii]);

    for (size_t ii = 0; ii < t->sources_used; ii++)
      free(t->sources[ii]);

    for (size_t ii = 0; ii < t->objects_used; ii++)
      free(t->objects[ii]);

    for (size_t ii = 0; ii < t->ldflags_used; ii++)
      free(t->ldflags[ii]);

    vector_free(t, cflags);
    vector_free(t, sources);
    vector_free(t, objects);
    vector_free(t, ldflags);
    vector_free(t, full_cmd);
    vector_free(t, deps);
    free(t);
  }

  vector_free(&b, deps);
}

#endif /* NOBITA_LINUX_IMPL */
