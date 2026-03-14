#ifndef LEVEL_PARSER_H
#define LEVEL_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "sokoban.h"

typedef struct {
  char *title;
  char *description;
  char *pack_metadata;
} ParsedLevelInfo;

bool parse_sok_level_from_file(FILE *file, size_t level_index, LevelState *out_level);
bool parse_sok_level_from_string(const char *text, size_t level_index, LevelState *out_level);
bool parse_sok_level_metadata_from_file(FILE *file, size_t level_index, char **out_metadata);
bool parse_sok_level_metadata_from_string(const char *text, size_t level_index, char **out_metadata);
bool parse_sok_level_title_from_file(FILE *file, size_t level_index, char **out_title);
bool parse_sok_level_title_from_string(const char *text, size_t level_index, char **out_title);
bool parse_sok_pack_metadata_from_file(FILE *file, char **out_metadata);
bool parse_sok_pack_metadata_from_string(const char *text, char **out_metadata);
bool parse_sok_level_info_from_file(FILE *file, size_t level_index, ParsedLevelInfo *out_info);
bool parse_sok_level_info_from_string(const char *text, size_t level_index, ParsedLevelInfo *out_info);
void free_parsed_level_info(ParsedLevelInfo *info);
bool count_sok_levels_in_file(FILE *file, size_t *out_count);
bool count_sok_levels_in_string(const char *text, size_t *out_count);

#endif // LEVEL_PARSER_H
