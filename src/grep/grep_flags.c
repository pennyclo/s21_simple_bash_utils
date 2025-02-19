#include "grep_flags.h"

void grep(data_t *data) {
  FILE *file = fopen(data->file_paths[data->value_counts.count_files], "r");
  if (file) {
    reader(file, data);

    fclose(file);
  } else if (!data->opt.s) {
    fprintf(stderr, "grep: %s: No such file or directory\n",
            data->file_paths[data->value_counts.count_files]);
  }
}

void reader(FILE *file, data_t *data) {
  char *line = NULL;
  size_t len = 0;
  data->num_lines = 1;
  data->value_counts.count_line = 0;
  data->value_counts.count_matchs = 0;
  int tmp_line = 0;

  while ((tmp_line = getline(&line, &len, file)) != -1) {
    outline(data, line, count_matchs(data, line));
  }

  processing_l_c_flags(data);

  free(line);
}

int count_matchs(data_t *data, const char *line) {
  int matchs_count = 0;

  for (int i = 0; i < data->num_pattern && !matchs_count && !data->invalid;
       i++) {
    compiling_reg(data, data->patterns[i]);

    if (!data->invalid) {
      matchs_count += valid_match(data, line);

      if (matchs_count) {
        data->value_counts.count_matchs++;
        data->valid_all_matchs = 1;
      }
    }

    regfree(&data->regex);
  }

  return matchs_count;
}

int valid_match(data_t *data, const char *line) {
  int match = 0;
  int value_match = 0;

  value_match = regexec(&data->regex, line, 0, NULL, 0);
  if (!value_match) {
    match = 1;
  } else if (value_match != REG_NOMATCH) {
    data->invalid = REG_ERROR;
  }

  return match;
}

void outline(data_t *data, const char *line, int matchs_count) {
  if (!data->opt.v) {
    if (matchs_count) {
      processing_line(data, line);
    }
  } else {
    if (!matchs_count) {
      processing_line(data, line);
    }
  }

  data->num_lines++;
}

void processing_line(data_t *data, const char *line) {
  if (data->opt.c || data->opt.l) {
    data->value_counts.count_line++;
  } else {
    if (data->opt.o) {
      processing_o_flag(data, line);
    } else {
      processing_h_flag(data);
      processing_n_flag(data);
      output(line);
    }
  }
}

void compiling_reg(data_t *data, const char *patterns) {
  int comp_reg = 0;

  if (data->opt.i) {
    comp_reg = regcomp(&data->regex, patterns, REG_ICASE);
  } else if (data->opt.o) {
    comp_reg = regcomp(&data->regex, patterns, REG_EXTENDED);
  } else {
    comp_reg = regcomp(&data->regex, patterns, REG_NEWLINE);
  }

  if (comp_reg) {
    data->invalid = COMPILE_REG;
  }
}

void output(const char *line) {
  if (line[strlen(line) - 1] == '\n') {
    printf("%s", line);
  } else {
    printf("%s\n", line);
  }
}

void processing_h_flag(const data_t *data) {
  if (data->num_files > 1 && !data->opt.h) {
    printf("%s:", data->file_paths[data->value_counts.count_files]);
  }
}

void processing_n_flag(const data_t *data) {
  if (data->opt.n) {
    printf("%d:", data->num_lines);
  }
}

void processing_c_flag(const data_t *data) {
  if (data->num_files > 1) {
    if (!data->opt.h) {
      printf("%s:%d\n", data->file_paths[data->value_counts.count_files],
             data->value_counts.count_line);
    } else {
      printf("%d\n", data->value_counts.count_line);
    }
  } else if (data->value_counts.count_matchs > 0) {
    printf("%d\n", data->value_counts.count_line);
  }
}

void processing_l_c_flags(const data_t *data) {
  if (!data->opt.v) {
    if (data->valid_all_matchs) {
      if (data->value_counts.count_line && data->opt.l) {
        printf("%s\n", data->file_paths[data->value_counts.count_files]);
      } else if (data->opt.c && !data->opt.l) {
        processing_c_flag(data);
      }
    }
  } else {
    if (data->value_counts.count_line && data->opt.l) {
      printf("%s\n", data->file_paths[data->value_counts.count_files]);
    } else if (data->opt.c && !data->opt.l) {
      processing_c_flag(data);
    }
  }
}

void processing_o_flag(data_t *data, const char *line) {
  regmatch_t pmatch[1];
  size_t len_patterns = find_len_patterns(data);

  char *patterns_line =
      patterns_for_o(data, alloc_patterns_o(data, len_patterns));

  int find_match = 0;
  int start = 0;

  compiling_reg(data, patterns_line);

  while ((find_match = regexec(&data->regex, line + start, 1, pmatch, 0)) ==
         0) {
    if (!data->opt.v) {
      processing_h_flag(data);
      processing_n_flag(data);
      printf("%.*s\n", (int)(pmatch[0].rm_eo - pmatch[0].rm_so),
             line + start + pmatch[0].rm_so);
    }
    start += pmatch[0].rm_eo;
  }

  if (find_match != REG_NOMATCH) {
    fprintf(stderr, "Regex match failed\n");
  }

  regfree(&data->regex);
  free(patterns_line);
}

size_t find_len_patterns(const data_t *data) {
  size_t len_patterns = 0;

  for (int i = 0; i < data->num_pattern; i++) {
    len_patterns += strlen(data->patterns[i]);
    if (i < data->num_pattern - 1) {
      len_patterns += 1;
    }
  }

  return len_patterns;
}

char *patterns_for_o(const data_t *data, char *patterns_line) {
  for (int i = 0; i < data->num_pattern; i++) {
    strcat(patterns_line, data->patterns[i]);
    if (i < data->num_pattern - 1) {
      strcat(patterns_line, "|");
    }
  }

  return patterns_line;
}
