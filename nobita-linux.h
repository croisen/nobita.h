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
typedef struct nobita_target Nobita_CMD;

enum nobita_argtype {
  NOBITA_T_CFLAGS,
  NOBITA_T_LDFLAGS,
  NOBITA_T_CUSTOM_CMD,
};

extern void build(Nobita_Build *b);

void Nobita_Try_Rebuild(Nobita_Build *b, const char *build_file);

void Nobita_Free_Later(Nobita_Build *b, void *ptr);

Nobita_Exe *Nobita_Build_Add_Exe(Nobita_Build *b, const char *name);
Nobita_Shared_Lib *Nobita_Build_Add_Shared_Lib(Nobita_Build *b,
                                               const char *name);
Nobita_Static_Lib *Nobita_Build_Add_Static_Lib(Nobita_Build *b,
                                               const char *name);
Nobita_CMD *Nobita_Build_Add_CMD(Nobita_Build *b, const char *name);

void Nobita_Target_Set_Build_Tools(struct nobita_target *t, const char *cc,
                                   const char *ar);
void Nobita_Target_Set_Build_Tool_Options(struct nobita_target *t,
                                          const char *cc_to_obj_opt,
                                          const char *cc_out_file_opt,
                                          const char *cc_shared_lib_opt,
                                          const char *ar_create_opts);
// void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix);
void Nobita_CMD_Add_Args(Nobita_CMD *c, ...);

#ifdef __GNUC__
void Nobita_Target_Add_Fmt_Arg(struct nobita_target *t, enum nobita_argtype a,
                               const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
#else
void Nobita_Target_Add_Fmt_Arg(struct nobita_target *t, const char *fmt, ...);
#endif /* __GNUC__ */

void Nobita_Target_Add_Cflags(struct nobita_target *t, ...);
void Nobita_Target_Add_Sources(struct nobita_target *t, ...);
void Nobita_Target_Add_LDflags(struct nobita_target *t, ...);
void Nobita_Target_Add_Deps(struct nobita_target *t, ...);
void Nobita_Target_Add_Headers(struct nobita_target *t, const char *parent,
                               ...);

#endif /* NOBITA_LINUX_H */

#define NOBITA_LINUX_IMPL
#ifdef NOBITA_LINUX_IMPL

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef pid_t nobita_pid;
#define NOBITA_PATHSEP "/"
#define NOBITA_EXECUT_EXT ".elf"
#define NOBITA_SHARED_EXT ".so"
#define NOBITA_STATIC_EXT ".a"

#endif /* _WIN32 */

enum nobita_target_type {
  NOBITA_EXECUTABLE,
  NOBITA_SHARED_LIB,
  NOBITA_STATIC_LIB,
  NOBITA_CUSTOM_CMD,
};

struct nobita_header {
  char *parent;
  char *header;
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

  size_t custom_cmd_used;
  size_t custom_cmd_size;
  char **custom_cmd;

  size_t headers_used;
  size_t headers_size;
  struct nobita_header *headers;

  size_t deps_used;
  size_t deps_size;
  void **deps;

  struct nobita_build *b;
};

struct nobita_build {
  size_t deps_used;
  size_t deps_size;
  void **deps;

  size_t free_later_used;
  size_t free_later_size;
  void **free_later;

  size_t proc_queue_used;
  size_t proc_queue_size;
  nobita_pid *proc_queue;

  int argc;
  char **argv;
  bool was_self_rebuilt;

  char *ced;
  char *cwd;
  char *prefix;
  char *include;
  char *bin;
  char *lib;
};

static struct nobita_target *nobita_build_add_target(struct nobita_build *b,
                                                     const char *name);

static nobita_pid nobita_proc_exec(char **cmd);
static int nobita_proc_wait(nobita_pid pid, bool pause);

static void nobita_proc_append(struct nobita_build *b, char **cmd);
static void nobita_proc_wait_one(struct nobita_build *b);
static void nobita_proc_wait_all(struct nobita_build *b);

static void nobita_mkdir_recursive(const char *path);
static bool nobita_is_a_newer(char *a, char *b);
static bool nobita_file_exist(char *path);
static bool nobita_dir_exist(char *path);

static void nobita_build_target(struct nobita_target *t);
static void nobita_cp(const char *dest, const char *src);
static char *nobita_strjoin(const char *join, ...);
static char *nobita_getcwd(void);
static char *nobita_getced(const char *arv0);
static char *nobita_strdup(const char *);
static void nobita_dirname(char *p);

static bool nobita_build_failed = false;
static size_t nobita_max_proc_count = 4;

#define vector_init(ptr, name)                                                 \
  do {                                                                         \
    if (nobita_build_failed)                                                   \
      break;                                                                   \
                                                                               \
    (ptr)->name##_size = 8;                                                    \
    (ptr)->name##_used = 0;                                                    \
    (ptr)->name = calloc(8, sizeof(*(ptr)->name));                             \
    if ((ptr)->name == NULL) {                                                 \
      fprintf(stderr, "Failed to initialize vector named %s\n", #name);        \
      nobita_build_failed = true;                                              \
    }                                                                          \
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
    if (nobita_build_failed)                                                   \
      break;                                                                   \
                                                                               \
    if ((ptr)->name##_size <= (ptr)->name##_used + 1) {                        \
      void *data = calloc((ptr)->name##_size * 2, sizeof(*(ptr)->name));       \
      if (data == NULL) {                                                      \
        fprintf(stderr,                                                        \
                "Failed to append data to vector named %s in struct %s\n",     \
                #name, #ptr);                                                  \
        nobita_build_failed = true;                                            \
        break;                                                                 \
      }                                                                        \
                                                                               \
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
    if (nobita_build_failed)                                                   \
      break;                                                                   \
                                                                               \
    for (size_t nobita_iter_##src_name = 0;                                    \
         nobita_iter_##src_name < (src)->src_name##_used;                      \
         nobita_iter_##src_name++)                                             \
      vector_append(dest, dest_name, (src)->src_name[nobita_iter_##src_name]); \
  } while (false)

void Nobita_Free_Later(Nobita_Build *b, void *ptr) {
  vector_append(b, free_later, ptr);
}

Nobita_Exe *Nobita_Build_Add_Exe(Nobita_Build *b, const char *name) {
  if (nobita_build_failed)
    return NULL;

  Nobita_Exe *e = nobita_build_add_target(b, name);
  if (e == NULL)
    goto ret;

  e->target_type = NOBITA_EXECUTABLE;

ret:
  return e;
}

Nobita_Shared_Lib *Nobita_Build_Add_Shared_Lib(Nobita_Build *b,
                                               const char *name) {
  if (nobita_build_failed)
    return NULL;

  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  if (l == NULL)
    goto ret;

  l->target_type = NOBITA_SHARED_LIB;

ret:
  return l;
}

Nobita_Static_Lib *Nobita_Build_Add_Static_Lib(Nobita_Build *b,
                                               const char *name) {
  if (nobita_build_failed)
    return NULL;

  Nobita_Shared_Lib *l = nobita_build_add_target(b, name);
  if (l == NULL)
    goto ret;

  l->target_type = NOBITA_STATIC_LIB;

ret:
  return l;
}

Nobita_CMD *Nobita_Add_CMD(Nobita_Build *b, const char *name) {
  if (nobita_build_failed)
    return NULL;

  Nobita_CMD *c = nobita_build_add_target(b, name);
  if (c == NULL)
    goto ret;

  c->target_type = NOBITA_CUSTOM_CMD;

ret:
  return c;
}

void Nobita_Target_Set_Build_Tools(struct nobita_target *t, const char *cc,
                                   const char *ar) {
  if (nobita_build_failed)
    return;

  if (cc == NULL) {
    fprintf(stderr, "Compiler for target %s has been set to NULL\n", t->name);
    nobita_build_failed = true;
    return;
  }

  t->cc = (char *)cc;

  if ((t->target_type == NOBITA_SHARED_LIB ||
       t->target_type == NOBITA_STATIC_LIB)) {
    if (ar == NULL) {
      fprintf(stderr, "Archiver for target library %s has been set to NULL\n",
              t->name);
      nobita_build_failed = true;
      return;
    }

    t->ar = (char *)ar;
  }
}

void Nobita_Target_Set_Build_Tool_Options(struct nobita_target *t,
                                          const char *cc_to_obj_opt,
                                          const char *cc_out_file_opt,
                                          const char *cc_shared_lib_opt,
                                          const char *ar_create_opts) {
  if (nobita_build_failed)
    return;

  if (cc_to_obj_opt == NULL || cc_out_file_opt == NULL) {
    fprintf(stderr, "One of the compiler options for target %s is NULL\n",
            t->name);
    nobita_build_failed = true;
    return;
  }

  t->cc_to_obj_opt = (char *)cc_to_obj_opt;
  t->cc_out_file_opt = (char *)cc_out_file_opt;

  if (t->target_type == NOBITA_STATIC_LIB) {
    if (ar_create_opts == NULL) {
      fprintf(stderr, "Archiver create options for target library %s is NULL\n",
              t->name);
      nobita_build_failed = true;
      return;
    }

    t->ar_create_opts = (char *)ar_create_opts;
  }

  if (t->target_type == NOBITA_SHARED_LIB) {
    if (cc_shared_lib_opt == NULL) {
      fprintf(stderr,
              "Compiler shared lib option for target library %s is NULL\n",
              t->name);
      nobita_build_failed = true;
      return;
    }

    t->cc_shared_lib_opt = (char *)cc_shared_lib_opt;
  }
}

// void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix) {
//   assert(prefix != NULL && "Please do not set prefix to NULL");
//   assert(*prefix == '/' && "Prefix path should be an absolute path");
//
//   t->prefix = malloc((strlen(prefix) + 1) * sizeof(char));
//   assert(t->prefix != NULL && "Buy more ram lol");
//   strcpy(t->prefix, prefix);
// }

void Nobita_Target_Add_Cflags(struct nobita_target *t, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    vector_append(t, cflags, arg);
    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_Sources(struct nobita_target *t, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *o =
        nobita_strjoin(NOBITA_PATHSEP, t->b->ced, "nobita-cache", arg, NULL);
    char *i = strrchr(o, '.');
    i[1] = 'o';
    i[2] = 0;

    char *o2 = nobita_strdup(o);
    if (o2 == NULL) {
      fprintf(stderr,
              "Could not get object counterpart of source: %s for target %s\n",
              arg, t->name);
      nobita_build_failed = true;
      va_end(va);
      return;
    }

    nobita_dirname(o2);
    nobita_mkdir_recursive(o2);

    vector_append(t, sources,
                  nobita_strjoin(NOBITA_PATHSEP, t->b->ced, arg, NULL));
    vector_append(t, objects, o);

    free(o2);
    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_LDflags(struct nobita_target *t, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, t);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    vector_append(t, ldflags, arg);
    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_Deps(struct nobita_target *t, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, t);

  struct nobita_target *arg = va_arg(va, struct nobita_target *);
  while (arg != NULL) {
    vector_append(t, deps, arg);
    arg = va_arg(va, struct nobita_target *);
  }

  va_end(va);
}

void Nobita_CMD_Add_Args(Nobita_CMD *c, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, c);

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    vector_append(c, custom_cmd, arg);
    arg = va_arg(va, char *);
  }

  va_end(va);
}

void Nobita_Target_Add_Fmt_Arg(struct nobita_target *t, enum nobita_argtype a,
                               const char *fmt, ...) {
  if (nobita_build_failed)
    return;

  va_list va;
  va_start(va, fmt);
  ssize_t arglen = vsnprintf(NULL, 0, fmt, va) + 1;
  va_end(va);

  char *arg = malloc(arglen * sizeof(char));
  if (arg == NULL) {
    fprintf(stderr, "Failed to add formatted arg to target %s\n", t->name);
    nobita_build_failed = true;
    return;
  }

  va_start(va, fmt);
  vsprintf(arg, fmt, va);
  va_end(va);

  switch (a) {
  case NOBITA_T_CFLAGS: {
    vector_append(t, cflags, arg);
    break;
  }
  case NOBITA_T_LDFLAGS: {
    vector_append(t, ldflags, arg);
    break;
  }
  case NOBITA_T_CUSTOM_CMD: {
    vector_append(t, custom_cmd, arg);
    break;
  }
  default:
    break;
  }

  Nobita_Free_Later(t->b, arg);
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

  nobita_pid id = nobita_proc_exec(e.full_cmd);
  nobita_proc_wait(id, true);
  e.full_cmd_used = 0;
  for (int i = 0; i < b->argc; i++)
    vector_append(&e, full_cmd, b->argv[i]);

  vector_append(&e, full_cmd, NULL);
  id = nobita_proc_exec(e.full_cmd);
  nobita_proc_wait(id, true);
  vector_free(&e, full_cmd);
  b->was_self_rebuilt = true;
}

void Nobita_Target_Add_Headers(struct nobita_target *t, const char *parent,
                               ...) {
  if (nobita_build_failed)
    return;

  if (parent == NULL) {
    fprintf(stderr,
            "When adding header files to a target, the parent "
            "dir should not be NULL (target: %s)\n",
            t->name);
    nobita_build_failed = true;
    return;
  }

  struct nobita_header h;
  h.parent = (char *)parent;

  va_list va;
  va_start(va, parent);
  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    h.header = arg;
    vector_append(t, headers, h);
    arg = va_arg(va, char *);
  }

  va_end(va);
}

static struct nobita_target *nobita_build_add_target(struct nobita_build *b,
                                                     const char *name) {
  if (nobita_build_failed)
    return NULL;

  if (name == NULL) {
    fprintf(stderr, "A target's name should not be NULL\n");
    nobita_build_failed = true;
    return NULL;
  }

  struct nobita_target *t = calloc(1, sizeof(*t));
  if (t == NULL) {
    fprintf(stderr, "Could not create target %s\n", name);
    nobita_build_failed = true;
    return NULL;
  }

  t->name = (char *)name;

  vector_init(t, cflags);
  vector_init(t, sources);
  vector_init(t, objects);
  vector_init(t, ldflags);
  vector_init(t, full_cmd);
  vector_init(t, custom_cmd);
  vector_init(t, headers);
  vector_init(t, deps);

  vector_append(b, deps, t);
  t->b = b;
  return t;
}

static nobita_pid nobita_proc_exec(char **cmd) {
  nobita_pid id = -1;

  if (nobita_build_failed)
    return id;

  id = fork();
  if (id == -1) {
    fprintf(stderr, "Fork failed\n");
    nobita_build_failed = true;
    return id;
  }

  if (id == 0) {
    execvp(*cmd, cmd);
    exit(errno);
  } else {
    return id;
  }
}

static int nobita_proc_wait(nobita_pid pid, bool pause) {
  int status = -1;
  waitpid(pid, &status, (pause) ? 0 : WNOHANG);
  return status;
}

static void nobita_proc_append(struct nobita_build *b, char **cmd) {
  if (nobita_build_failed)
    return;

  if (b->proc_queue_used >= nobita_max_proc_count)
    nobita_proc_wait_one(b);

  if (nobita_build_failed)
    return;

  char **c = cmd;
  while (true) {
    printf("%s", *c);
    c += 1;
    if (*c != NULL)
      printf(" ");
    else
      break;
  }

  printf("\n");
  nobita_pid pid = nobita_proc_exec(cmd);
  vector_append(b, proc_queue, pid);
}

static void nobita_proc_wait_one(struct nobita_build *b) {
  bool continue_loop = !nobita_build_failed;
  while (continue_loop) {
    for (size_t i = 0; i < b->proc_queue_used; i++) {
      int status = nobita_proc_wait(b->proc_queue[i], false);
      if (status != -1 && WIFEXITED(status)) {
        nobita_pid p = b->proc_queue[i];
        b->proc_queue[i] = b->proc_queue[b->proc_queue_used - 1];
        b->proc_queue[b->proc_queue_used - 1] = p;
        b->proc_queue_used -= 1;

        status = nobita_proc_wait(p, true);
        if (WEXITSTATUS(status) != EXIT_SUCCESS) {
          nobita_build_failed = true;
          fprintf(
              stderr,
              "A process did not exit sucessfully, marking build as failed\n");
        }

        continue_loop = false;
        break;
      }
    }
  }
}

static void nobita_proc_wait_all(struct nobita_build *b) {
  for (size_t i = 0; i < b->proc_queue_used; i++) {
    int status = nobita_proc_wait(b->proc_queue[i], true);
    if (!WIFEXITED(status) && !nobita_build_failed) {
      nobita_build_failed = true;
      fprintf(stderr, "A process did not exit successfully\n");
    }

    if (WEXITSTATUS(status) != EXIT_SUCCESS && !nobita_build_failed) {
      nobita_build_failed = true;
      fprintf(stderr, "A process did not exit successfully\n");
    }
  }

  b->proc_queue_used = 0;
}

static void nobita_mkdir_recursive(const char *path) {
  if (nobita_build_failed)
    return;

  char *p2 = nobita_strdup(path);
  if (p2 == NULL) {
    fprintf(stderr, "Could not copy path '%s' for mkdir recursive\n", path);
    nobita_build_failed = true;
    return;
  }

  size_t len = strlen(p2);
  for (size_t i = 1; i < len; i++) {
    if (p2[i] == *NOBITA_PATHSEP) {
      p2[i] = 0;

      if (!nobita_dir_exist(p2)) {
        printf("nobita_mkdir %s\n", p2);
        mkdir(p2, 0777);
      }

      p2[i] = *NOBITA_PATHSEP;
    }
  }

  if (!nobita_dir_exist(p2)) {
    printf("nobita_mkdir %s\n", p2);
    mkdir(p2, 0777);
  }

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

static bool nobita_file_exist(char *path) {
  struct stat buf;
  if (stat(path, &buf) == -1)
    return false;

  return !S_ISDIR(buf.st_mode);
}

static bool nobita_dir_exist(char *path) {
  struct stat buf;
  if (stat(path, &buf) == -1)
    return false;

  return S_ISDIR(buf.st_mode);
}

static void nobita_build_target(struct nobita_target *t) {
  if (t->built || nobita_build_failed)
    return;

  bool rebuild = false;
  for (size_t i = 0; i < t->deps_used; i++) {
    nobita_build_target(t->deps[i]);
    rebuild = (!rebuild) ? ((struct nobita_target *)t->deps[i])->rebuilt : true;
  }

  nobita_proc_wait_all(t->b);
  if (t->target_type != NOBITA_CUSTOM_CMD)
    for (size_t i = 0; i < t->objects_used; i++) {
      if (!nobita_is_a_newer(t->objects[i], t->sources[i]) || rebuild) {
        vector_append(t, full_cmd, t->cc);
        vector_append_vector(t, full_cmd, t, cflags);
        vector_append(t, full_cmd, t->cc_out_file_opt);
        vector_append(t, full_cmd, t->objects[i]);
        vector_append(t, full_cmd, t->cc_to_obj_opt);
        vector_append(t, full_cmd, t->sources[i]);
        vector_append(t, full_cmd, NULL);

        if (t->sources_used > 0)
          nobita_proc_append(t->b, t->full_cmd);

        t->full_cmd_used = 0;
        rebuild = true;
      }
    }

  nobita_proc_wait_all(t->b);
  char *name = NULL;
  char *output = NULL;

  switch (t->target_type) {
  case NOBITA_EXECUTABLE: {
    name = nobita_strjoin("", t->name, NOBITA_EXECUT_EXT, NULL);
    output = nobita_strjoin(NOBITA_PATHSEP, t->b->bin, name, NULL);
    break;
  }
  case NOBITA_SHARED_LIB: {
    name = nobita_strjoin("", "lib", t->name, NOBITA_SHARED_EXT, NULL);
    output = nobita_strjoin(NOBITA_PATHSEP, t->b->lib, name, NULL);
    break;
  }
  case NOBITA_STATIC_LIB: {
    name = nobita_strjoin("", "lib", t->name, NOBITA_STATIC_EXT, NULL);
    output = nobita_strjoin(NOBITA_PATHSEP, t->b->lib, name, NULL);
    break;
  }
  case NOBITA_CUSTOM_CMD: {
    break;
  }
  }

  if (!rebuild)
    for (size_t i = 0; i < t->objects_used; i++)
      rebuild = (!rebuild) ? nobita_is_a_newer(t->objects[i], output) : true;

  if (!rebuild && t->target_type != NOBITA_CUSTOM_CMD)
    goto end;

  switch (t->target_type) {
  case NOBITA_EXECUTABLE: {
    vector_append(t, full_cmd, t->cc);
    vector_append_vector(t, full_cmd, t, cflags);
    vector_append(t, full_cmd, t->cc_out_file_opt);
    vector_append(t, full_cmd, output);
    vector_append_vector(t, full_cmd, t, objects);
    vector_append_vector(t, full_cmd, t, ldflags);
    break;
  }
  case NOBITA_SHARED_LIB: {
    vector_append(t, full_cmd, t->cc);
    vector_append_vector(t, full_cmd, t, cflags);
    vector_append(t, full_cmd, t->cc_shared_lib_opt);
    vector_append(t, full_cmd, t->cc_out_file_opt);
    vector_append(t, full_cmd, output);
    vector_append_vector(t, full_cmd, t, objects);
    vector_append_vector(t, full_cmd, t, ldflags);
    break;
  }
  case NOBITA_STATIC_LIB: {
    vector_append(t, full_cmd, t->ar);
    vector_append(t, full_cmd, t->ar_create_opts);
    vector_append(t, full_cmd, output);
    vector_append_vector(t, full_cmd, t, objects);
    break;
  }
  case NOBITA_CUSTOM_CMD: {
    vector_append_vector(t, full_cmd, t, custom_cmd);
    break;
  }
  }

  vector_append(t, full_cmd, NULL);
  if (t->sources_used > 0 || t->target_type == NOBITA_CUSTOM_CMD)
    nobita_proc_append(t->b, t->full_cmd);

  if (t->target_type != NOBITA_EXECUTABLE) {
    for (size_t i = 0; i < t->headers_used; i++) {
      char *parent = t->headers[i].parent;
      char *header = t->headers[i].header;
      char *header_cp =
          nobita_strjoin(NOBITA_PATHSEP, t->b->include, header, NULL);
      nobita_dirname(header_cp);
      nobita_mkdir_recursive(header_cp);

      char *src = nobita_strjoin(NOBITA_PATHSEP, parent, header, NULL);
      char *dest = nobita_strjoin(NOBITA_PATHSEP, t->b->include, header, NULL);
      nobita_cp(dest, src);

      free(header_cp);
      free(src);
      free(dest);
    }
  }

  t->rebuilt = true;

end:
  t->built = true;
  free(name);
  free(output);
}

static void nobita_cp(const char *dest, const char *src) {
  if (nobita_build_failed)
    return;

  FILE *d = fopen(dest, "w");
  if (d == NULL) {
    fprintf(stderr, "Failed to open dest file '%s' in copying\n", dest);
    nobita_build_failed = true;
    return;
  }

  FILE *s = fopen(src, "r");
  if (s == NULL) {
    fprintf(stderr, "Failed to open source file '%s' in copying\n", src);
    nobita_build_failed = true;
    fclose(d);
    return;
  }

  char buf[4096];
  while (true) {
    size_t read = fread(buf, sizeof(char), 4096, s);
    if (!read)
      break;

    size_t write = fwrite(buf, sizeof(char), read, d);
    if (read != write) {
      fprintf(
          stderr,
          "Read and write counts for copying files %s -> %s did not match\n",
          src, dest);
      nobita_build_failed = true;
      goto end;
    }
  }

  printf("nobita_cp %s %s\n", src, dest);

end:
  fclose(d);
  fclose(s);
}

static char *nobita_strjoin(const char *join, ...) {
  if (nobita_build_failed)
    return NULL;

  char *ret = NULL;
  size_t len = 1;

  char *arg = NULL;
  va_list va;
  va_start(va, join);
  arg = va_arg(va, char *);
  while (arg != NULL) {
    len += strlen(arg);
    arg = va_arg(va, char *);
    if (arg != NULL)
      len += strlen(join);
  }
  va_end(va);

  ret = malloc(len * sizeof(char));
  if (ret == NULL) {
    fprintf(stderr,
            "Could not join strings using strjoin as malloc returned NULL\n");
    nobita_build_failed = true;
    return ret;
  }

  va_start(va, join);
  *ret = 0;
  arg = va_arg(va, char *);
  while (arg != NULL) {
    strcat(ret, arg);
    arg = va_arg(va, char *);
    if (arg != NULL)
      strcat(ret, join);
  }
  va_end(va);

  return ret;
}

static char *nobita_getcwd(void) {
  char *ret = getcwd(NULL, 0);
  if (ret == NULL) {
    fprintf(stderr, "Couldn't get current working directory\n");
    nobita_build_failed = true;
  }

  return ret;
}

static char *nobita_getced(const char *argv0) {
  char *ret = NULL;
  char *exe = nobita_strdup(argv0);
  char *cwd = nobita_getcwd();
  nobita_dirname(exe);
  chdir(exe);
  ret = nobita_getcwd();
  chdir(cwd);
  free(exe);
  free(cwd);

  return ret;
}

static char *nobita_strdup(const char *s) {
  char *ret = malloc((strlen(s) + 1) * sizeof(char));
  if (ret == NULL) {
    fprintf(stderr, "Could not duplicate a string as malloc returned NULL\n");
    nobita_build_failed = true;
  } else {
    strcpy(ret, s);
  }

  return ret;
}

static void nobita_dirname(char *p) {
  char *s = strrchr(p, *NOBITA_PATHSEP);
  if (s == NULL) {
    p[0] = '.';
    p[1] = 0;
    return;
  }

  *s = 0;
}

int main(int argc, char **argv) {
  struct nobita_build b;

  char *ced = nobita_getced(argv[0]);
  char *cwd = nobita_getcwd();
  char *prefix = nobita_strjoin(NOBITA_PATHSEP, cwd, "nobita-build", NULL);
  char *include = nobita_strjoin(NOBITA_PATHSEP, prefix, "include", NULL);
  char *bin = nobita_strjoin(NOBITA_PATHSEP, prefix, "bin", NULL);
  char *lib = nobita_strjoin(NOBITA_PATHSEP, prefix, "lib", NULL);

  nobita_mkdir_recursive(prefix);
  nobita_mkdir_recursive(bin);
  nobita_mkdir_recursive(lib);
  nobita_mkdir_recursive(include);

  vector_init(&b, deps);
  vector_init(&b, free_later);
  vector_init(&b, proc_queue);

  b.argc = argc;
  b.argv = argv;
  b.was_self_rebuilt = false;

  b.ced = ced;
  b.cwd = cwd;
  b.prefix = prefix;
  b.include = include;
  b.bin = bin;
  b.lib = lib;

  build(&b);

  if (!b.was_self_rebuilt)
    for (size_t i = 0; i < b.deps_used; i++)
      nobita_build_target(b.deps[i]);

  for (size_t i = 0; i < b.deps_used; i++) {
    struct nobita_target *t = b.deps[i];
    for (size_t ii = 0; ii < t->objects_used; ii++) {
      free(t->sources[ii]);
      free(t->objects[ii]);
    }

    vector_free(t, cflags);
    vector_free(t, sources);
    vector_free(t, objects);
    vector_free(t, ldflags);
    vector_free(t, full_cmd);
    vector_free(t, custom_cmd);
    vector_free(t, headers);
    vector_free(t, deps);
    free(t);
  }

  nobita_proc_wait_all(&b);

  for (size_t i = 0; i < b.free_later_used; i++)
    free(b.free_later[i]);

  vector_free(&b, deps);
  vector_free(&b, free_later);
  vector_free(&b, proc_queue);

  free(ced);
  free(cwd);
  free(prefix);
  free(include);
  free(bin);
  free(lib);
}

#endif /* NOBITA_LINUX_IMPL */
