#include "edushell.h"
#include <sys/stat.h>
#include <time.h>

bool move_to_trash(const char *path, ShellState *state) {
    char trash_path[MAX_PATH_LENGTH];
    time_t now = time(NULL);
    
    // Get base filename
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    
    // Create unique name in trash
    snprintf(trash_path, sizeof(trash_path), "%s/%d_%s", 
             state->trash_dir, (int)(now % 1000000), base);
    
    // Move file to trash
    if (rename(path, trash_path) != 0) {
        handle_error("Could not move file to trash");
        return false;
    }
    
    // Add to trash list
    DeletedFile *df = malloc(sizeof(DeletedFile));
    if (!df) {
        handle_error("Memory allocation failed");
        return false;
    }
    
    strncpy(df->original_path, path, MAX_PATH_LENGTH-1);
    strncpy(df->trash_path, trash_path, MAX_PATH_LENGTH-1);
    df->deletion_time = now;
    df->next = state->trash_list;
    state->trash_list = df;
    state->trash_count++;
    
    printf("Moved '%s' to trash\n", path);
    return true;
}

bool restore_from_trash(const char *path, ShellState *state) {
    DeletedFile *prev = NULL;
    DeletedFile *current = state->trash_list;
    
    while (current) {
        if (strcmp(current->original_path, path) == 0) {
            // Restore file
            if (rename(current->trash_path, current->original_path) != 0) {
                handle_error("Could not restore file");
                return false;
            }
            
            // Remove from trash list
            if (prev) {
                prev->next = current->next;
            } else {
                state->trash_list = current->next;
            }
            
            printf("Restored '%s'\n", path);
            free(current);
            state->trash_count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    printf("File '%s' not found in trash\n", path);
    return false;
} 