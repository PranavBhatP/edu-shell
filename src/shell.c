#include "edushell.h"

void initialize_shell(ShellState *state) {
    // Initialize shell state
    state->history_count = 0;
    state->tutorial_mode = false;
    state->trash_list = NULL;
    state->trash_count = 0;
    
    // Create trash directory
    snprintf(state->trash_dir, MAX_PATH_LENGTH, "%s/.edushell_trash", getenv("HOME"));
    mkdir(state->trash_dir, 0700);
    
    // Open log file
    char log_path[MAX_PATH_LENGTH];
    snprintf(log_path, MAX_PATH_LENGTH, "%s/.edushell_log", getenv("HOME"));
    state->log_file = fopen(log_path, "a");
}

void shell_loop(ShellState *state) {
    char *line;
    Command *cmd;
    int running = 1;  // Changed from status to running to be more clear
    int status;       // New variable for command status

    while (running) {
        printf(COLOR_GREEN SHELL_PROMPT COLOR_RESET);
        line = read_line();

        if (!line) continue;

        // Add command to history
        if (state->history_count < HISTORY_SIZE) {
            state->history[state->history_count++] = strdup(line);
        }

        cmd = parse_command(line);
        if (cmd) {
            if (!handle_builtin(cmd, state)) {
                status = execute_command(cmd, state);
                if (status != 0) {
                    // Command failed, try to suggest alternatives
                    suggest_command(cmd->args[0]);
                }
            }
            free_command(cmd);
        }

        free(line);
    }
}

char *read_line(void) {
    char *line = NULL;
    size_t bufsize = 0;
    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            printf("\n");
            exit(0);
        } else {
            handle_error("Error reading input");
            return NULL;
        }
    }
    // Remove trailing newline
    line[strcspn(line, "\n")] = 0;
    return line;
}

Command *parse_command(char *line) {
    Command *cmd = malloc(sizeof(Command));
    if (!cmd) {
        handle_error("Memory allocation error");
        return NULL;
    }

    cmd->arg_count = 0;
    cmd->is_background = false;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = false;

    char *token = strtok(line, " \t");
    while (token && cmd->arg_count < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t");
            if (token) cmd->input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t");
            if (token) cmd->output_file = strdup(token);
        } else if (strcmp(token, ">>") == 0) {
            token = strtok(NULL, " \t");
            if (token) {
                cmd->output_file = strdup(token);
                cmd->append_output = true;
            }
        } else if (strcmp(token, "&") == 0) {
            cmd->is_background = true;
            break;
        } else {
            cmd->args[cmd->arg_count++] = strdup(token);
        }
        token = strtok(NULL, " \t");
    }
    cmd->args[cmd->arg_count] = NULL;
    return cmd;
} 