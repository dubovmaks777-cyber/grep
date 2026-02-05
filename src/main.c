#include "grep.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  GrepOptions options;
  PatternList patterns;
  int first_file = 0;

  if (!parse_flags(argc, argv, &options, &patterns, &first_file)) {
    patterns_free(&patterns);
    fprintf(stderr, "Usage: %s [OPTIONS] PATTERN [FILE...]\n", argv[0]);
    return 2;
  }

  char *joined = join_patterns(&patterns);
  if (!joined) {
    patterns_free(&patterns);
    fprintf(stderr, "grep: failed to allocate pattern\n");
    return 2;
  }

  regex_t regex;
  int reg_flags = REG_EXTENDED;
  if (options.ignore_case) {
    reg_flags |= REG_ICASE;
  }
  int reg_result = regcomp(&regex, joined, reg_flags);
  if (reg_result != 0) {
    char buffer[256];
    regerror(reg_result, &regex, buffer, sizeof(buffer));
    fprintf(stderr, "grep: %s\n", buffer);
    free(joined);
    patterns_free(&patterns);
    return 2;
  }

  bool has_files = first_file < argc;
  bool multiple_files = (argc - first_file) > 1;
  bool show_filename = multiple_files && !options.no_filename;

  int exit_status = 1;
  if (!has_files) {
    size_t match_count = 0;
    bool matched = process_stream(stdin, "(standard input)", &regex, &options, false,
                                  options.count_only ? &match_count : NULL);
    if (options.list_files) {
      if (matched) {
        printf("%s\n", "(standard input)");
        exit_status = 0;
      }
    } else if (options.count_only) {
      printf("%zu\n", match_count);
      if (match_count > 0) {
        exit_status = 0;
      }
    } else if (matched) {
      exit_status = 0;
    }
  } else {
    for (int i = first_file; i < argc; ++i) {
      const char *path = argv[i];
      FILE *file = fopen(path, "r");
      if (!file) {
        if (!options.suppress_errors) {
          fprintf(stderr, "grep: %s: %s\n", path, strerror(errno));
        }
        continue;
      }
      size_t match_count = 0;
      bool matched = process_stream(file, path, &regex, &options, show_filename,
                                    options.count_only ? &match_count : NULL);
      fclose(file);
      if (options.list_files) {
        if (matched) {
          printf("%s\n", path);
          exit_status = 0;
        }
      } else if (options.count_only) {
        if (show_filename) {
          printf("%s:%zu\n", path, match_count);
        } else {
          printf("%zu\n", match_count);
        }
        if (match_count > 0) {
          exit_status = 0;
        }
      } else if (matched) {
        exit_status = 0;
      }
    }
  }

  regfree(&regex);
  free(joined);
  patterns_free(&patterns);
  return exit_status;
}
