#include "edushell.h"

bool execute_script(const char *filename, ShellState *state) {
    FILE *script = fopen(filename, "r");
    if (!script) {
        handle_error("Could not open script file");
        return false;
    }

    char line[MAX_COMMAND_LENGTH];
    int line_number = 0;

    printf("Executing script: %s\n", filename);

    while (fgets(line, sizeof(line), script)) {
        line_number++;
        
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        // Parse and execute the command
        Command *cmd = parse_command(line);
        if (cmd) {
            printf(COLOR_GREEN "Script[%d]> %s\n" COLOR_RESET, line_number, line);
            
            if (!handle_builtin(cmd, state)) {
                int status = execute_command(cmd, state);
                if (status != 0) {
                    printf(COLOR_RED "Script error at line %d\n" COLOR_RESET, line_number);
                }
            }
            free_command(cmd);
        }
    }

    fclose(script);
    return true;
} 