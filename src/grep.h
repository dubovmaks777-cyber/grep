#ifndef S21_GREP_H
#define S21_GREP_H

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
  bool ignore_case;
  bool invert_match;
  bool count_only;
  bool list_files;
  bool line_number;
  bool no_filename;
  bool suppress_errors;
  bool only_matching;
} GrepOptions;

typedef struct {
  char **items;
  size_t count;
  size_t capacity;
} PatternList;

void patterns_init(PatternList *list);
void patterns_free(PatternList *list);
bool patterns_push(PatternList *list, const char *pattern);
bool read_patterns_from_file(PatternList *list, const char *path, bool suppress_errors);
char *join_patterns(const PatternList *list);

bool parse_flags(int argc, char **argv, GrepOptions *options, PatternList *patterns,
                 int *first_file_index);

bool process_stream(FILE *stream, const char *filename, regex_t *regex,
                    const GrepOptions *options, bool show_filename, size_t *match_count);

#endif
