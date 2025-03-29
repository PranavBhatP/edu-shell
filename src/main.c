#include "edushell.h"

int main(int argc, char *argv[]) {
    ShellState state;
    initialize_shell(&state);

    if (argc > 1) {
        // Check if file ends with .esh
        const char *ext = strrchr(argv[1], '.');
        if (ext && strcmp(ext, ".esh") == 0) {
            execute_script(argv[1], &state);
        } else {
            printf("Error: Script file must have .esh extension\n");
        }
        cleanup_shell(&state);
        return 0;
    }

    // Interactive mode
    printf("Welcome to EduShell - An educational shell for learning!\n");
    printf("Type 'help' for usage information or 'tutorial' to start the tutorial mode.\n\n");
    shell_loop(&state);

    cleanup_shell(&state);
    return 0;
} 