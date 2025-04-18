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
        // Handle I/O redirection
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