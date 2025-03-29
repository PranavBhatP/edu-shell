#include "edushell.h"
#include <sys/stat.h>

void handle_error(const char *message) {
    fprintf(stderr, COLOR_RED "Error: %s\n" COLOR_RESET, message);
    if (errno != 0) {
        fprintf(stderr, COLOR_RED "System error: %s\n" COLOR_RESET, strerror(errno));
    }
}

void free_command(Command *cmd) {
    if (!cmd) return;
    
    for (int i = 0; i < cmd->arg_count; i++) {
        free(cmd->args[i]);
    }
    free(cmd->input_file);
    free(cmd->output_file);
    free(cmd);
}

void cleanup_shell(ShellState *state) {
    // Free history
    for (int i = 0; i < state->history_count; i++) {
        free(state->history[i]);
    }
    
    // Close log file
    if (state->log_file) {
        fclose(state->log_file);
    }
}

void log_command(ShellState *state, const char *command, int status) {
    if (state->log_file) {
        time_t now = time(NULL);
        char *timestamp = ctime(&now);
        // Remove newline from timestamp
        timestamp[strlen(timestamp) - 1] = '\0';
        fprintf(state->log_file, "[%s] Command: %s (Status: %d)\n", 
                timestamp, command, status);
        fflush(state->log_file);
    }
}

bool handle_builtin(Command *cmd, ShellState *state) {
    if (!cmd || cmd->arg_count == 0) return false;

    const char *command = cmd->args[0];

    // Add tutorial command
    if (strcmp(command, "tutorial") == 0) {
        start_tutorial(state);
        return true;
    }

    // exit command
    if (strcmp(command, "exit") == 0) {
        cleanup_shell(state);
        exit(0);
    }
    
    // cd command
    if (strcmp(command, "cd") == 0) {
        if (cmd->arg_count < 2) {
            // Change to home directory if no argument
            chdir(getenv("HOME"));
        } else if (chdir(cmd->args[1]) != 0) {
            handle_error("Could not change directory");
        }
        return true;
    }

    // pwd command
    if (strcmp(command, "pwd") == 0) {
        char cwd[MAX_PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            handle_error("Could not get current directory");
        }
        return true;
    }

    // history command
    if (strcmp(command, "history") == 0) {
        for (int i = 0; i < state->history_count; i++) {
            printf("%d  %s\n", i + 1, state->history[i]);
        }
        return true;
    }

    // clear command
    if (strcmp(command, "clear") == 0) {
        printf("\033[H\033[J");  // ANSI escape sequence to clear screen
        return true;
    }

    // echo command
    if (strcmp(command, "echo") == 0) {
        FILE *output = stdout;
        int end = cmd->arg_count;
        
        // Check for output redirection
        if (end >= 3 && strcmp(cmd->args[end-2], ">") == 0) {
            output = fopen(cmd->args[end-1], "w");
            if (!output) {
                handle_error("Could not open output file");
                return true;
            }
            end -= 2;
        }
        
        for (int i = 1; i < end; i++) {
            fprintf(output, "%s ", cmd->args[i]);
        }
        fprintf(output, "\n");
        
        if (output != stdout) {
            fclose(output);
        }
        return true;
    }

    // help command
    if (strcmp(command, "help") == 0) {
        printf("EduShell - Available Commands:\n\n");
        printf("Built-in commands:\n");
        printf("  cd [dir]     - Change directory (empty for home)\n");
        printf("  pwd          - Print working directory\n");
        printf("  history      - Show command history\n");
        printf("  clear        - Clear the screen\n");
        printf("  echo [text]  - Print text to screen\n");
        printf("  tutorial     - Start interactive tutorial\n");
        printf("  exit         - Exit the shell\n");
        printf("  help         - Show this help message\n");
        printf("\nFeatures:\n");
        printf("  - Command auto-correction\n");
        printf("  - Command history\n");
        printf("  - Background processes with &\n");
        printf("  - Error handling with suggestions\n");
        printf("  - Command logging\n");
        return true;
    }

    // type command (shows if command is builtin or external)
    if (strcmp(command, "type") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: type command_name\n");
            return true;
        }
        
        const char *cmd_name = cmd->args[1];
        if (strcmp(cmd_name, "cd") == 0 || 
            strcmp(cmd_name, "pwd") == 0 ||
            strcmp(cmd_name, "history") == 0 ||
            strcmp(cmd_name, "clear") == 0 ||
            strcmp(cmd_name, "echo") == 0 ||
            strcmp(cmd_name, "help") == 0 ||
            strcmp(cmd_name, "type") == 0 ||
            strcmp(cmd_name, "exit") == 0) {
            printf("%s is a shell builtin\n", cmd_name);
        } else {
            // Check if command exists in PATH
            char path[MAX_PATH_LENGTH];
            if (find_command_path(cmd_name, path, MAX_PATH_LENGTH)) {
                printf("%s is %s\n", cmd_name, path);
            } else {
                printf("%s not found\n", cmd_name);
            }
        }
        return true;
    }

    // rm command
    if (strcmp(command, "rm") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: rm <file>\n");
            return true;
        }
        // Move to trash instead of deleting
        return move_to_trash(cmd->args[1], state);
    }

    // restore command
    if (strcmp(command, "restore") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: restore <file>\n");
            return true;
        }
        return restore_from_trash(cmd->args[1], state);
    }

    // trash-list command
    if (strcmp(command, "trash-list") == 0) {
        DeletedFile *current = state->trash_list;
        if (!current) {
            printf("Trash is empty\n");
            return true;
        }
        
        printf("Files in trash:\n");
        while (current) {
            char time_str[26];
            ctime_r(&current->deletion_time, time_str);
            time_str[24] = '\0';  // Remove newline
            printf("%s (deleted on %s)\n", current->original_path, time_str);
            current = current->next;
        }
        return true;
    }

    return false;
}

// Helper function to find command path
bool find_command_path(const char *cmd, char *path_buf, size_t buf_size) {
    char *path_env = getenv("PATH");
    if (!path_env) return false;

    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");

    while (dir) {
        snprintf(path_buf, buf_size, "%s/%s", dir, cmd);
        if (access(path_buf, X_OK) == 0) {
            free(path_copy);
            return true;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return false;
}

int execute_command(Command *cmd, ShellState *state) {
    if (!cmd || cmd->arg_count == 0) return 1;

    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        if (cmd->input_file) {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                handle_error("Could not open input file");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        if (cmd->output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= cmd->append_output ? O_APPEND : O_TRUNC;
            int fd = open(cmd->output_file, flags, 0644);
            if (fd < 0) {
                handle_error("Could not open output file");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(cmd->args[0], cmd->args);
        handle_error("Command execution failed");
        exit(1);
    } else if (pid < 0) {
        handle_error("Fork failed");
        return 1;
    }

    // Parent process
    if (!cmd->is_background) {
        int status;
        waitpid(pid, &status, 0);
        log_command(state, cmd->args[0], status);
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    return 0;
} 