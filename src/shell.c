#define _GNU_SOURCE
#include "edushell.h"
#include "analytics.h"
#include <time.h>

void initialize_shell(ShellState *state) {
    state->history_count = 0;
    state->tutorial_mode = false;
    state->trash_list = NULL;
    state->trash_count = 0;
    state->monitor_mode = false;
    state->analytics_enabled = true; 
    state->sandbox_enabled = false;
    

    snprintf(state->trash_dir, MAX_PATH_LENGTH, "%s/.edushell_trash", getenv("HOME"));
    mkdir(state->trash_dir, 0700);
    

    char log_path[MAX_PATH_LENGTH];
    snprintf(log_path, MAX_PATH_LENGTH, "%s/.edushell_log", getenv("HOME"));
    state->log_file = fopen(log_path, "a");

    initialize_analytics();
}

void shell_loop(ShellState *state) {
    char *line;
    Command *cmd;
    int status;
    struct timespec end_time;

    while (1) {
        // Update resource usage if monitoring is enabled
        if (state->monitor_mode) {
            update_resource_usage();
            display_resource_graphs();
        }

        printf(COLOR_GREEN SHELL_PROMPT COLOR_RESET);
        line = read_line();

        if (!line) continue;

        // Add command to history
        if (state->history_count < HISTORY_SIZE) {
            state->history[state->history_count++] = strdup(line);
        }

        cmd = parse_command(line);
        if (cmd) {
            // Record start time for command execution
            clock_gettime(CLOCK_MONOTONIC, &cmd->start_time);

            if (!handle_builtin(cmd, state)) {
                status = execute_command(cmd, state);
                
                // Calculate execution time and track analytics
                if (state->analytics_enabled) {
                    clock_gettime(CLOCK_MONOTONIC, &end_time);
                    double execution_time = 
                        (end_time.tv_sec - cmd->start_time.tv_sec) +
                        (end_time.tv_nsec - cmd->start_time.tv_nsec) / 1e9;
                    
                    track_command_execution(cmd->args[0], execution_time, status != 0);
                }

                if (status != 0) {
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

bool handle_builtin(Command *cmd, ShellState *state) {
    if (!cmd || cmd->arg_count == 0) return false;

    const char *command = cmd->args[0];
    struct timespec end_time;

    // Add monitor command
    if (strcmp(command, "monitor") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: monitor [on|off]\n");
            return true;
        }
        
        if (strcmp(cmd->args[1], "on") == 0) {
            state->monitor_mode = true;
            printf("Resource monitoring enabled\n");
        } else if (strcmp(cmd->args[1], "off") == 0) {
            state->monitor_mode = false;
            printf("Resource monitoring disabled\n");
        }
        return true;
    }

    // Add analytics command
    if (strcmp(command, "analytics") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: analytics [show|on|off]\n");
            return true;
        }
        
        if (strcmp(cmd->args[1], "show") == 0) {
            display_learning_dashboard();
        } else if (strcmp(cmd->args[1], "on") == 0) {
            state->analytics_enabled = true;
            printf("Learning analytics enabled\n");
        } else if (strcmp(cmd->args[1], "off") == 0) {
            state->analytics_enabled = false;
            printf("Learning analytics disabled\n");
        }
        return true;
    }

    // sandbox command
    if (strcmp(command, "sandbox") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: sandbox [on|off]\n");
            return true;
        }
        
        if (strcmp(cmd->args[1], "on") == 0) {
            if (geteuid() != 0) {
                printf("Sandbox mode requires root privileges\n");
                return true;
            }

            if (state->sandbox_enabled) {
                printf("Sandbox is already enabled\n");
                return true;
            }
            
            // Create sandbox environment path
            snprintf(state->sandbox_root, MAX_PATH_LENGTH, 
                    "%s/.edushell_sandbox", getenv("HOME"));
            
            // Create sandbox environment first
            if (!create_sandbox_env(state->sandbox_root)) {
                printf("Failed to create sandbox environment\n");
                return true;
            }

            // Setup namespaces (this will fork)
            pid_t child_pid = setup_sandbox();
            if (child_pid == -1) {
                printf("Failed to setup sandbox namespaces\n");
                return true;
            }

            if (child_pid > 0) {
                // Parent process - wait for child to exit
                int status;
                waitpid(child_pid, &status, 0);
                return true;
            }

            // We are now in the child process (PID 1 in new namespace)
            
            // Change root to sandbox environment
            if (chroot(state->sandbox_root) != 0) {
                handle_error("Failed to change root to sandbox");
                _exit(1);
            }
            
            // Change to home directory inside sandbox
            if (chdir("/home/user") != 0) {
                handle_error("Failed to change directory in sandbox");
                _exit(1);
            }

            //Now mount proc inside the new root
            if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
                handle_error("Failed to mount proc filesystem");
                _exit(1);
            }

            snprintf(state->trash_dir, MAX_PATH_LENGTH, "/home/user/.edushell_trash");
            
            state->sandbox_enabled = true;
            printf("Sandbox mode enabled\n");
            shell_loop(state);
            _exit(0);        } else if (strcmp(cmd->args[1], "off") == 0) {
            if (state->sandbox_enabled) {
                //cannot exit chroot once entered.
                printf("Cannot disable sandbox once enabled. Please start a new shell.\n");
                return true;
            }
        }
        return true;
    }
    if (strcmp(command, "exit") == 0) {
        cleanup_shell(state);
        exit(0);
    }

    if (strcmp(command, "cd") == 0) {
        if (cmd->arg_count < 2) {
            // Change to home directory if no argument
            chdir(getenv("HOME"));
        } else if (chdir(cmd->args[1]) != 0) {
            handle_error("Could not change directory");
        }
        return true;
    }

    if (strcmp(command, "pwd") == 0) {
        char cwd[MAX_PATH_LENGTH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            handle_error("Could not get current directory");
        }
        return true;
    }

    if (strcmp(command, "history") == 0) {
        for (int i = 0; i < state->history_count; i++) {
            printf("%d  %s\n", i + 1, state->history[i]);
        }
        return true;
    }

    if (strcmp(command, "clear") == 0) {
        printf("\033[H\033[J");  // ANSI escape sequence to clear screen
        return true;
    }

    if (strcmp(command, "rm") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: rm <file>\n");
            return true;
        }
        
        // Move file to trash instead of deleting
        if (!move_to_trash(cmd->args[1], state)) {
            handle_error("Failed to move file to trash");
        }
        return true;
    }

    if (strcmp(command, "restore") == 0) {
        if (cmd->arg_count < 2) {
            printf("Usage: restore <file>\n");
            return true;
        }
        
        if (!restore_from_trash(cmd->args[1], state)) {
            handle_error("Failed to restore file");
        }
        return true;
    }

    if (strcmp(command, "trash-list") == 0) {
        if (state->trash_count == 0) {
            printf("Trash is empty\n");
            return true;
        }

        printf("Files in trash:\n");
        printf("%-40s %-30s\n", "Original Path", "Deletion Time");
        printf("---------------------------------------- ------------------------------\n");
        
        DeletedFile *current = state->trash_list;
        while (current) {
            char time_str[30];
            struct tm *tm_info = localtime(&current->deletion_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("%-40s %-30s\n", current->original_path, time_str);
            current = current->next;
        }
        return true;
    }

    if (strcmp(command, "help") == 0) {
        printf("EduShell - Available Commands:\n\n");
        printf("Built-in commands:\n");
        printf("  cd [dir]     - Change directory (empty for home)\n");
        printf("  pwd          - Print working directory\n");
        printf("  history      - Show command history\n");
        printf("  clear        - Clear the screen\n");
        printf("  echo [text]  - Print text to screen\n");
        printf("  sandbox      - Enable/disable sandbox mode\n");
        printf("  monitor      - Enable/disable resource monitoring\n");
        printf("  analytics    - Show/control learning analytics\n");
        printf("  trash-list   - List files in trash\n");
        printf("  restore      - Restore file from trash\n");
        printf("  exit         - Exit the shell\n");
        printf("  help         - Show this help message\n");
        return true;
    }

    // Track execution time for built-in commands
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double execution_time = 
        (end_time.tv_sec - cmd->start_time.tv_sec) +
        (end_time.tv_nsec - cmd->start_time.tv_nsec) / 1e9;

    if (state->analytics_enabled) {
        track_command_execution(command, execution_time, false);
    }

    return false;
} 