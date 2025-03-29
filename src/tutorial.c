#include "edushell.h"

typedef struct {
    const char *title;
    const char *description;
    const char *task;
    const char *success_message;
    const char *hint;
    bool (*check_completion)(const char *input);
} TutorialStep;

static bool check_pwd(const char *input) {
    return strcmp(input, "pwd") == 0;
}

static bool check_ls(const char *input) {
    return strcmp(input, "ls") == 0 || strncmp(input, "ls ", 3) == 0;
}

static bool check_cd(const char *input) {
    return strcmp(input, "cd") == 0 || strncmp(input, "cd ", 3) == 0;
}

static bool check_mkdir(const char *input) {
    return strncmp(input, "mkdir ", 6) == 0;
}

static bool check_echo_redirect(const char *input) {
    return strstr(input, "echo") && strstr(input, ">");
}

static bool check_cat(const char *input) {
    return strncmp(input, "cat ", 4) == 0;
}

static TutorialStep tutorial_steps[] = {
    {
        "Navigation - Present Working Directory",
        "Let's start with basic navigation. First, let's find out where we are.",
        "Type 'pwd' to print your current working directory.",
        "Great! 'pwd' shows your current location in the filesystem.",
        "Hint: pwd = Print Working Directory",
        check_pwd
    },
    {
        "Navigation - Listing Files",
        "Now let's see what files and directories are here.",
        "Use 'ls' to list the contents of the current directory.",
        "Perfect! 'ls' shows all files and directories. Try 'ls -l' for more details!",
        "Hint: ls = LiSt directory contents",
        check_ls
    },
    {
        "Navigation - Changing Directories",
        "Let's learn to move between directories.",
        "Use 'cd' to go to your home directory.",
        "Well done! 'cd' without arguments takes you home. Try 'cd ..' to go up one level.",
        "Hint: cd = Change Directory",
        check_cd
    },
    {
        "File Operations - Creating Directories",
        "Let's create a new directory for practice.",
        "Create a directory named 'tutorial_test' using mkdir.",
        "Excellent! You can now use 'cd tutorial_test' to enter it.",
        "Hint: mkdir = MaKe DIRectory",
        check_mkdir
    },
    {
        "File Operations - Creating Files",
        "Let's create a file with some content.",
        "Use echo and redirection (>) to create a file. Try: echo \"Hello World\" > test.txt",
        "Perfect! You've created a file using redirection.",
        "Hint: The > symbol redirects output to a file",
        check_echo_redirect
    },
    {
        "File Operations - Reading Files",
        "Now let's read the content of our new file.",
        "Use the 'cat' command to read test.txt",
        "Great! 'cat' displays file contents. You can also use 'less' for longer files.",
        "Hint: cat = conCATenate and display",
        check_cat
    },
    {NULL, NULL, NULL, NULL, NULL, NULL}
};

void start_tutorial(ShellState *state) {
    state->tutorial_mode = true;
    int current_step = 0;
    
    printf("\n%sWelcome to the EduShell Tutorial!%s\n", COLOR_GREEN, COLOR_RESET);
    printf("This tutorial will guide you through basic shell usage.\n");
    printf("Type 'hint' for help or 'exit_tutorial' to end the tutorial.\n\n");
    
    while (tutorial_steps[current_step].title && state->tutorial_mode) {
        TutorialStep *step = &tutorial_steps[current_step];
        
        printf("\n%s=== %s ===%s\n", COLOR_GREEN, step->title, COLOR_RESET);
        printf("%s\n", step->description);
        printf("%sTask: %s%s\n", COLOR_GREEN, step->task, COLOR_RESET);
        
        while (1) {
            printf("%sTutorial> %s", COLOR_GREEN, COLOR_RESET);
            char *input = read_line();
            if (!input) continue;
            
            if (strcmp(input, "exit_tutorial") == 0) {
                state->tutorial_mode = false;
                free(input);
                break;
            }
            
            if (strcmp(input, "hint") == 0) {
                printf("%s%s%s\n", COLOR_GREEN, step->hint, COLOR_RESET);
                free(input);
                continue;
            }
            
            if (step->check_completion(input)) {
                printf("%s%s%s\n", COLOR_GREEN, step->success_message, COLOR_RESET);
                free(input);
                current_step++;
                break;
            } else {
                printf("Try again! Type 'hint' for help.\n");
            }
            
            free(input);
        }
        
        if (!state->tutorial_mode) break;
    }
    
    if (!state->tutorial_mode) {
        printf("\nTutorial ended. Type 'tutorial' to restart.\n");
    } else {
        printf("\n%sCongratulations! You've completed the tutorial!%s\n", COLOR_GREEN, COLOR_RESET);
        printf("You've learned the basics of shell navigation and file operations.\n");
        printf("Keep exploring and try combining these commands in new ways!\n");
    }
    state->tutorial_mode = false;
} 