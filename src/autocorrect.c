#include "edushell.h"
#include <limits.h>
// Common Unix commands for suggestions
static const char *COMMON_COMMANDS[] = {
    "ls", "cd", "pwd", "mkdir", "rm", "cp", "mv", "cat", "grep",
    "echo", "touch", "chmod", "chown", "man", "find", "ps",
    "kill", "top", "clear", "history", "exit", "help",NULL
};

int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    int matrix[len1 + 1][len2 + 1];
    

    for (int i = 0; i <= len1; i++)
        matrix[i][0] = i;
    for (int j = 0; j <= len2; j++)
        matrix[0][j] = j;

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            
            matrix[i][j] = MIN3(
                matrix[i-1][j] + 1,      
                matrix[i][j-1] + 1,      
                matrix[i-1][j-1] + cost  
            );
        }
    }
    
    return matrix[len1][len2];
}

void suggest_command(const char *input) {
    if (!input || strlen(input) == 0) return;
    
    const char *best_match = NULL;
    int min_distance = INT_MAX;
    
    // Find the command with minimum Levenshtein distance
    for (int i = 0; COMMON_COMMANDS[i] != NULL; i++) {
        int distance = levenshtein_distance(input, COMMON_COMMANDS[i]);
        if (distance < min_distance && distance <= 3) {  // Only suggest if distance ≤ 3
            min_distance = distance;
            best_match = COMMON_COMMANDS[i];
        }
    }
    
    if (best_match) {
        printf(COLOR_GREEN "Did you mean '%s'? (y/n): " COLOR_RESET, best_match);
        char response = getchar();
        // Clear input buffer
        while (getchar() != '\n');
        
        if (response == 'y' || response == 'Y') {
            // Show command usage hint
            printf("\n");
            if (strcmp(best_match, "ls") == 0)
                printf("Usage: ls [OPTION]... [FILE]... - list directory contents\n");
            else if (strcmp(best_match, "cd") == 0)
                printf("Usage: cd [dir] - change directory (empty for home)\n");
            else if (strcmp(best_match, "pwd") == 0)
                printf("Usage: pwd - print working directory\n");
            else if (strcmp(best_match, "rm") == 0)
                printf("Usage: rm <file> - remove file (moves to trash)\n");
            else if (strcmp(best_match, "help") == 0)
                printf("Usage: help - show available commands and features\n");
            // Add more command hints as needed
        }
    }
} 