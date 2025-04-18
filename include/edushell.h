#ifndef EDUSHELL_H
#define EDUSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/capability.h>
#include <linux/capability.h>
#include "analytics.h"

#define SHELL_PROMPT "EduShell> "
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64
#define MAX_PATH_LENGTH 256
#define HISTORY_SIZE 100

// Color codes for output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

// Add this macro near the other #defines
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

// Structure to hold command information
typedef struct {
    char *args[MAX_ARGS];
    int arg_count;
    bool is_background;
    char *input_file;    // For <
    char *output_file;   // For >
    bool append_output;  // For >>
    struct timespec start_time;  // For tracking execution time
} Command;

// Add this before the ShellState structure
typedef struct DeletedFile {
    char original_path[MAX_PATH_LENGTH];
    char trash_path[MAX_PATH_LENGTH];
    time_t deletion_time;
    struct DeletedFile *next;
} DeletedFile;

// Structure for shell state
typedef struct {
    char *history[HISTORY_SIZE];
    int history_count;
    char trash_dir[MAX_PATH_LENGTH];
    bool tutorial_mode;
    FILE *log_file;
    struct DeletedFile *trash_list;
    int trash_count;
    bool sandbox_enabled;
    char sandbox_root[MAX_PATH_LENGTH];
    bool monitor_mode;  // For resource monitoring
    bool analytics_enabled;  // For learning analytics
} ShellState;

// Function prototypes
void initialize_shell(ShellState *state);
void cleanup_shell(ShellState *state);
void shell_loop(ShellState *state);
char *read_line(void);
Command *parse_command(char *line);
int execute_command(Command *cmd, ShellState *state);
void free_command(Command *cmd);
void handle_error(const char *message);
bool handle_builtin(Command *cmd, ShellState *state);
void log_command(ShellState *state, const char *command, int status);
void suggest_command(const char *input);
int levenshtein_distance(const char *s1, const char *s2);
bool find_command_path(const char *cmd, char *path_buf, size_t buf_size);
bool execute_script(const char *filename, ShellState *state);
bool move_to_trash(const char *path, ShellState *state);
bool restore_from_trash(const char *path, ShellState *state);
void start_tutorial(ShellState *state);
bool create_sandbox_env(const char *sandbox_root);
pid_t setup_sandbox(void);

#endif 