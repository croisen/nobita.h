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
// void Nobita_Target_Set_Prefix(struct nobita_target *t, const char *prefix);

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

#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum nobita_target_type {
  NOBITA_EXECUTABLE,
  NOBITA_SHARED_LIB,
  NOBITA_STATIC_LIB,
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

  size_t headers_used;
  size_t headers_size;
  struct nobita_header *headers;

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
static void nobita_mkdir_recursive(const char *path);
static bool nobita_is_a_newer(char *a, char *b);
static bool nobita_file_exist(char *path);
static bool nobita_dir_exist(char *path);

static void nobita_build_target(struct nobita_target *t);
static char *nobita_dir_append(const char *prefix, ...);
static void nobita_cp(const char *dest, const char *src);

static bool nobita_build_failed = false;

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
  char *cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    fprintf(stderr, "Could not get current working directory\n");
    nobita_build_failed = true;
    return;
  }

  char *arg = va_arg(va, char *);
  while (arg != NULL) {
    char *o = nobita_dir_append("nobita-cache", arg, NULL);
    char *i = strrchr(o, '.');
    i[1] = 'o';
    i[2] = 0;

    char *o2 = strdup(o);
    if (o2 == NULL) {
      fprintf(stderr,
              "Could not get object counterpart of source: %s for target %s\n",
              arg, t->name);
      nobita_build_failed = true;
      free(cwd);
      va_end(va);
      return;
    }

    char *o3 = dirname(o2);
    nobita_mkdir_recursive(o3);

    vector_append(t, sources, arg);
    vector_append(t, objects, o);

    free(o2);
    arg = va_arg(va, char *);
  }

  free(cwd);
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
  vector_init(t, headers);
  vector_init(t, deps);

  vector_append(b, deps, t);
  return t;
}

static void nobita_fork_exec(char **cmd) {
  if (nobita_build_failed)
    return;

  pid_t id = fork();
  if (id == -1) {
    fprintf(stderr, "Fork failed\n");
    nobita_build_failed = true;
    return;
  }

  if (id == 0) {
    execvp(*cmd, cmd);
    exit(errno);
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

    int status;
    printf("%s\n", full_cmd);
    waitpid(id, &status, 0);
    if (!WIFEXITED(status)) {
      fprintf(stderr, "Process '%s' did not exit properly\n", full_cmd);
      nobita_build_failed = true;
      goto end;
    }

    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
      fprintf(stderr, "Process '%s' exitted unsuccessfully\n", full_cmd);
      nobita_build_failed = true;
      goto end;
    }

  end:
    free(full_cmd);
  }
}

static char *nobita_dir_append(const char *prefix, ...) {
  if (nobita_build_failed)
    return NULL;

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
  if (append == NULL) {
    fprintf(stderr, "Failed to allocate memory for str concat\n");
    nobita_build_failed = true;
    return NULL;
  }

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
  if (nobita_build_failed)
    return;

  char *p2 = strdup(path);
  if (p2 == NULL) {
    fprintf(stderr, "Could not copy path '%s' for mkdir recursive\n", path);
    nobita_build_failed = true;
    return;
  }

  size_t len = strlen(p2);
  for (size_t i = 1; i < len; i++) {
    if (p2[i] == '/') {
      p2[i] = 0;

      if (!nobita_dir_exist(p2))
        mkdir(p2, 0777);

      p2[i] = '/';
    }
  }

  if (!nobita_dir_exist(p2))
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
  if (name == NULL) {
    fprintf(stderr, "Could not copy target name '%s' in build target\n",
            t->name);
    nobita_build_failed = true;
    return;
  }

  char *cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    fprintf(stderr,
            "Could get current working directory for target name '%s' in build "
            "target\n",
            t->name);
    nobita_build_failed = true;
    return;
  }

  char *prefix = nobita_dir_append(cwd, "nobita-build", NULL);
  char *bin = nobita_dir_append(prefix, "bin", NULL);
  char *lib = nobita_dir_append(prefix, "lib", NULL);
  char *include = nobita_dir_append(prefix, "include", NULL);
  char *output = NULL;

  nobita_mkdir_recursive(prefix);
  nobita_mkdir_recursive(bin);
  nobita_mkdir_recursive(lib);
  nobita_mkdir_recursive(include);

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

  if (t->target_type != NOBITA_EXECUTABLE) {
    for (size_t i = 0; i < t->headers_used; i++) {
      char *parent = t->headers[i].parent;
      char *header = t->headers[i].header;
      char *header_cp = nobita_dir_append(include, header, NULL);
      char *header_dir = dirname(header_cp);
      nobita_mkdir_recursive(header_dir);

      char *src = nobita_dir_append(parent, header, NULL);
      char *dest = nobita_dir_append(include, header, NULL);
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
  free(cwd);
  free(prefix);
  free(bin);
  free(lib);
  free(include);
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
    for (size_t ii = 0; ii < t->objects_used; ii++)
      free(t->objects[ii]);

    vector_free(t, cflags);
    vector_free(t, sources);
    vector_free(t, objects);
    vector_free(t, ldflags);
    vector_free(t, full_cmd);
    vector_free(t, headers);
    vector_free(t, deps);
    free(t);
  }

  vector_free(&b, deps);
}

#endif /* NOBITA_LINUX_IMPL */
