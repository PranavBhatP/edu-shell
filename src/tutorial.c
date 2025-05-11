#include "edushell.h"
#include <stdio.h>
#include <string.h>

void start_tutorial(ShellState *state) {
    printf("Welcome to the EduShell Tutorial!\n");
    printf("This tutorial will guide you through basic shell commands and features.\n");
    printf("Type the command as instructed or 'exit' to quit the tutorial.\n\n");

    char input[256];
    int step = 0;
    while (1) {
        switch (step) {
            case 0:
                printf("Step 1: Let's start with the 'pwd' command.\n");
                printf("Type 'pwd' to print the current working directory.\n");
                break;
            case 1:
                printf("Great! Now let's try changing directories.\n");
                printf("Type 'cd ..' to move up one directory level.\n");
                break;
            case 2:
                printf("Well done! You can also create directories.\n");
                printf("Type 'mkdir test' to create a new directory named 'test'.\n");
                break;
            case 3:
                printf("Excellent! Let's remove that directory.\n");
                printf("Type 'rmdir test' to remove the 'test' directory.\n");
                break;
            case 4:
                printf("You're doing great! Let's list files in the current directory.\n");
                printf("Type 'ls' to list directory contents.\n");
                break;
            case 5:
                printf("Fantastic! You've completed the basic tutorial.\n");
                printf("Type 'exit' to leave the tutorial or 'pwd' to repeat.\n");
                break;
            default:
                printf("Tutorial complete. Type 'exit' to leave.\n");
                break;
        }

        printf("EduShell Tutorial> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;  // Remove newline character

        if (strcmp(input, "exit") == 0) {
            printf("Exiting tutorial.\n");
            break;
        } else if ((step == 0 && strcmp(input, "pwd") == 0) ||
                   (step == 1 && strcmp(input, "cd ..") == 0) ||
                   (step == 2 && strcmp(input, "mkdir test") == 0) ||
                   (step == 3 && strcmp(input, "rmdir test") == 0) ||
                   (step == 4 && strcmp(input, "ls") == 0)) {
            // Parse the command
            Command *cmd = parse_command(input);
            if (cmd) {
                // Try built-in commands first
                if (!handle_builtin(cmd, state)) {
                    // If not a built-in command, execute it normally
                    execute_command(cmd, state);
                }
                free_command(cmd);
            }
            step++;
        } else {
            printf("Invalid command. Please follow the instructions for each step.\n");
        }
    }
} 