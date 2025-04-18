#define _GNU_SOURCE
#include <sched.h>
#include "edushell.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <linux/fs.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>

static bool mkdir_p(const char *path) {
    char tmp[MAX_PATH_LENGTH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    // Create parent directories
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    // Create the final directory
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return false;
    }

    return true;
}

static bool create_sandbox_directories(const char *root) {
    char dirs[][MAX_PATH_LENGTH] = {
        "/bin", "/usr/bin", "/lib", "/lib64",
        "/usr/lib", "/usr/lib64", "/tmp"
    };

    for (size_t i = 0; i < sizeof(dirs)/sizeof(dirs[0]); i++) {
        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s%s", root, dirs[i]);
        if (!mkdir_p(path)) {
            handle_error("Failed to create sandbox directory");
            return false;
        }
    }
    return true;
}

bool create_sandbox_env(const char *sandbox_root) {
    // Create sandbox root if it doesn't exist
    if (!mkdir_p(sandbox_root)) {
        handle_error("Failed to create sandbox root");
        return false;
    }

    // Make sure sandbox root is empty
    if (mount("none", sandbox_root, "tmpfs", 0, NULL) != 0) {
        handle_error("Failed to mount sandbox root");
        return false;
    }

    // Create necessary directories
    if (!create_sandbox_directories(sandbox_root)) {
        return false;
    }

    // Create sandbox home directory and trash
    char home_path[PATH_MAX], trash_path[PATH_MAX];
    snprintf(home_path, sizeof(home_path), "%s/home/user", sandbox_root);
    snprintf(trash_path, sizeof(trash_path), "%s/home/user/.edushell_trash", sandbox_root);
    
    if (!mkdir_p(home_path) || !mkdir_p(trash_path)) {
        handle_error("Failed to create home or trash directory");
        return false;
    }

    // Set permissions
    if (chmod(home_path, 0755) != 0 || chmod(trash_path, 0700) != 0) {
        handle_error("Failed to set directory permissions");
        return false;
    }

    // Bind mount essential directories
    const char *bind_paths[] = {
        "/bin", "/usr/bin", "/lib", "/lib64",
        "/usr/lib", "/usr/lib64", "/usr/share",
        "/etc", NULL
    };

    for (const char **path = bind_paths; *path; path++) {
        char dst[PATH_MAX];
        snprintf(dst, sizeof(dst), "%s%s", sandbox_root, *path);
        
        // Skip if source doesn't exist
        struct stat st;
        if (stat(*path, &st) != 0) {
            continue;
        }
        
        // Create the target directory
        if (!mkdir_p(dst)) {
            handle_error("Failed to create mount point directory");
            return false;
        }

        // Perform the bind mount
        if (mount(*path, dst, NULL, MS_BIND | MS_RDONLY | MS_REC, NULL) != 0) {
            handle_error("Failed to bind mount directory");
            return false;
        }

        // Make the bind mount private and read-only
        if (mount(NULL, dst, NULL, MS_PRIVATE | MS_REMOUNT | MS_BIND | MS_RDONLY | MS_REC, NULL) != 0) {
            handle_error("Failed to make mount private and read-only");
            return false;
        }
    }

    // Create /proc directory
    char proc_path[PATH_MAX];
    snprintf(proc_path, sizeof(proc_path), "%s/proc", sandbox_root);
    if (!mkdir_p(proc_path)) {
        handle_error("Failed to create /proc directory");
        return false;
    }

    return true;
}

// Return values: -1 on error, 0 if child process, >0 if parent process (returns child's pid)
pid_t setup_sandbox(void) {
    // Create new namespaces
    if (unshare(CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID) != 0) {
        handle_error("Failed to create new namespaces");
        return -1;
    }

    // Make root mount private to prevent mount propagation
    if (mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) != 0) {
        handle_error("Failed to make root mount private");
        return -1;
    }

    // Fork to become PID 1 in the new PID namespace
    pid_t child = fork();
    if (child < 0) {
        handle_error("Failed to fork for PID namespace");
        return -1;
    }

    // Return child pid to parent, 0 to child
    return child;
} 