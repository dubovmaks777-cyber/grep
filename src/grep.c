#include "grep.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

void patterns_init(PatternList *list) {
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

void patterns_free(PatternList *list) {
  for (size_t i = 0; i < list->count; ++i) {
    free(list->items[i]);
  }
  free(list->items);
  list->items = NULL;
  list->count = 0;
  list->capacity = 0;
}

bool patterns_push(PatternList *list, const char *pattern) {
  if (list->count == list->capacity) {
    size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
    char **new_items = realloc(list->items, new_capacity * sizeof(char *));
    if (!new_items) {
      return false;
    }
    list->items = new_items;
    list->capacity = new_capacity;
  }
  list->items[list->count] = strdup(pattern);
  if (!list->items[list->count]) {
    return false;
  }
  list->count += 1;
  return true;
}

bool read_patterns_from_file(PatternList *list, const char *path, bool suppress_errors) {
  FILE *file = fopen(path, "r");
  if (!file) {
    if (!suppress_errors) {
      fprintf(stderr, "grep: %s: %s\n", path, strerror(errno));
    }
    return false;
  }
  char *line = NULL;
  size_t size = 0;
  ssize_t length = 0;
  while ((length = getline(&line, &size, file)) != -1) {
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
      line[length - 1] = '\0';
      length--;
    }
    if (!patterns_push(list, line)) {
      free(line);
      fclose(file);
      return false;
    }
  }
  free(line);
  fclose(file);
  return true;
}

char *join_patterns(const PatternList *list) {
  if (list->count == 0) {
    return NULL;
  }
  size_t total = 0;
  for (size_t i = 0; i < list->count; ++i) {
    total += strlen(list->items[i]) + 1;
  }
  char *joined = malloc(total);
  if (!joined) {
    return NULL;
  }
  joined[0] = '\0';
  for (size_t i = 0; i < list->count; ++i) {
    strcat(joined, list->items[i]);
    if (i + 1 < list->count) {
      strcat(joined, "|");
    }
  }
  return joined;
}

bool parse_flags(int argc, char **argv, GrepOptions *options, PatternList *patterns,
                 int *first_file_index) {
  patterns_init(patterns);
  memset(options, 0, sizeof(*options));

  int i = 1;
  bool pattern_from_arg = false;
  for (; i < argc; ++i) {
    const char *arg = argv[i];
    if (arg[0] != '-' || strcmp(arg, "-") == 0) {
      break;
    }
    if (strcmp(arg, "--") == 0) {
      i++;
      break;
    }
    for (size_t j = 1; arg[j] != '\0'; ++j) {
      char flag = arg[j];
      if (flag == 'e' || flag == 'f') {
        const char *value = NULL;
        if (arg[j + 1] != '\0') {
          value = &arg[j + 1];
          j = strlen(arg) - 1;
        } else if (i + 1 < argc) {
          value = argv[++i];
        }
        if (!value) {
          return false;
        }
        if (flag == 'e') {
          if (!patterns_push(patterns, value)) {
            return false;
          }
          pattern_from_arg = true;
        } else {
          if (!read_patterns_from_file(patterns, value, options->suppress_errors)) {
            return false;
          }
          pattern_from_arg = true;
        }
      } else if (flag == 'i') {
        options->ignore_case = true;
      } else if (flag == 'v') {
        options->invert_match = true;
      } else if (flag == 'c') {
        options->count_only = true;
      } else if (flag == 'l') {
        options->list_files = true;
      } else if (flag == 'n') {
        options->line_number = true;
      } else if (flag == 'h') {
        options->no_filename = true;
      } else if (flag == 's') {
        options->suppress_errors = true;
      } else if (flag == 'o') {
        options->only_matching = true;
      } else {
        return false;
      }
    }
  }

  if (!pattern_from_arg) {
    if (i >= argc) {
      return false;
    }
    if (!patterns_push(patterns, argv[i])) {
      return false;
    }
    i++;
  }
  *first_file_index = i;
  return true;
}

static void print_prefix(const char *filename, bool show_filename, size_t line_number,
                         bool show_line_number) {
  if (show_filename) {
    printf("%s:", filename);
  }
  if (show_line_number) {
    printf("%zu:", line_number);
  }
}

static bool line_matches(regex_t *regex, const char *line) {
  return regexec(regex, line, 0, NULL, 0) == 0;
}

static void output_matches(regex_t *regex, const char *line, const char *filename,
                           bool show_filename, size_t line_number, bool show_line_number) {
  const char *cursor = line;
  regmatch_t match;
  while (regexec(regex, cursor, 1, &match, 0) == 0) {
    size_t start = (size_t)match.rm_so;
    size_t end = (size_t)match.rm_eo;
    print_prefix(filename, show_filename, line_number, show_line_number);
    fwrite(cursor + start, 1, end - start, stdout);
    printf("\n");
    if (end == start) {
      if (cursor[end] == '\0') {
        break;
      }
      cursor += end + 1;
    } else {
      cursor += end;
    }
  }
}

bool process_stream(FILE *stream, const char *filename, regex_t *regex,
                    const GrepOptions *options, bool show_filename, size_t *match_count) {
  char *line = NULL;
  size_t size = 0;
  ssize_t length = 0;
  size_t line_number = 0;
  bool matched = false;
  while ((length = getline(&line, &size, stream)) != -1) {
    line_number++;
    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
      line[length - 1] = '\0';
      length--;
    }
    bool is_match = line_matches(regex, line);
    if (options->invert_match) {
      is_match = !is_match;
    }
    if (is_match) {
      matched = true;
      if (match_count) {
        (*match_count)++;
      }
      if (options->list_files) {
        break;
      }
      if (options->count_only) {
        continue;
      }
      if (options->only_matching && !options->invert_match) {
        output_matches(regex, line, filename, show_filename, line_number, options->line_number);
      } else {
        print_prefix(filename, show_filename, line_number, options->line_number);
        printf("%s\n", line);
      }
    }
  }
  free(line);
  return matched;
}
