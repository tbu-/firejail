#ifndef UTILS_H
#define UTILS_H

// run a user program using popen; returns static memory
char *run_program(const char *prog);

// returns true or false if the program was found using "which" shell command
bool which(const char *prog);

// check if a name.desktop file exists in config home directory
bool have_config_file(const char *name);

// get a coniguration file path based on the name; returns malloced memory
char *get_config_file_name(const char *name);

// get the full path of the home directory; returns malloced memory
char *get_home_directory();

#endif