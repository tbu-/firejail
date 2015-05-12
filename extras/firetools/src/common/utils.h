#ifndef UTILS_H
#define UTILS_H

char *run_program(const char *prog);
bool which(const char *prog);
bool have_config_file(const char *name);
char *get_config_file_name(const char *name);
char *get_home_directory();

#endif