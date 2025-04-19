#define _GNU_SOURCE
#include "analytics.h"
#include "edushell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/times.h>
#include <time.h>
#include <sys/time.h>

static ResourceHistory resource_history;
static LearningStats learning_stats;
static struct timespec last_update_time;
static unsigned long long last_idle = 0, last_total = 0;

static const char *GRAPH_CHARS[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

void initialize_analytics(void) {
    memset(&resource_history, 0, sizeof(ResourceHistory));
    memset(&learning_stats, 0, sizeof(LearningStats));
    learning_stats.session_start = time(NULL);
    clock_gettime(CLOCK_MONOTONIC, &last_update_time);
    last_idle = last_total = 0;
}

double get_cpu_usage(void) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;

    unsigned long long user, nice, system, idle, iowait, irq, softirq;
    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq) != 7) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);

    unsigned long long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long long current_idle = idle + iowait;

    unsigned long long total_delta = total - last_total;
    unsigned long long idle_delta = current_idle - last_idle;

    last_total = total;
    last_idle = current_idle;

    if (total_delta == 0) return 0.0;
    return 100.0 * (1.0 - ((double)idle_delta / total_delta));
}

double get_memory_usage(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0.0;

    unsigned long total = 0, free = 0, buffers = 0, cached = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0)
            sscanf(line, "MemTotal: %lu", &total);
        else if (strncmp(line, "MemFree:", 8) == 0)
            sscanf(line, "MemFree: %lu", &free);
        else if (strncmp(line, "Buffers:", 8) == 0)
            sscanf(line, "Buffers: %lu", &buffers);
        else if (strncmp(line, "Cached:", 7) == 0) {
            sscanf(line, "Cached: %lu", &cached);
            break;
        }
    }
    fclose(fp);

    if (total == 0) return 0.0;
    unsigned long used = total - free - buffers - cached;
    return (100.0 * used) / total;
}

double get_disk_io(void) {
    struct statvfs fs;
    if (statvfs("/", &fs) != 0) return 0.0;
    
    unsigned long total = fs.f_blocks;
    unsigned long free = fs.f_bfree;
    if (total == 0) return 0.0;
    
    return 100.0 * (1.0 - ((double)free / total));
}

void update_resource_usage(void) {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // Only update every UPDATE_INTERVAL seconds
    double elapsed = (current_time.tv_sec - last_update_time.tv_sec) +
                    (current_time.tv_nsec - last_update_time.tv_nsec) / 1e9;
    
    if (elapsed < UPDATE_INTERVAL) return;

    ResourcePoint point = {
        .timestamp = time(NULL),
        .cpu_usage = get_cpu_usage(),
        .memory_usage = get_memory_usage(),
        .disk_io = get_disk_io()
    };
    
    resource_history.points[resource_history.current_index] = point;
    resource_history.current_index = (resource_history.current_index + 1) % MAX_RESOURCE_POINTS;
    if (resource_history.total_points < MAX_RESOURCE_POINTS) {
        resource_history.total_points++;
    }

    last_update_time = current_time;
}

static void draw_graph(const char *title, double values[], int count) {
    // Save cursor position
    printf("\033[s");
    
    printf("\n%s:\n", title);
    
    // Find min and max values
    double max_val = 0.0, min_val = 100.0;
    for (int i = 0; i < count; i++) {
        if (values[i] > max_val) max_val = values[i];
        if (values[i] < min_val) min_val = values[i];
    }
    if (max_val == min_val) max_val = min_val + 1;
    
    // Draw graph
    for (int y = GRAPH_HEIGHT - 1; y >= 0; y--) {
        double threshold = (y * max_val) / (GRAPH_HEIGHT - 1);
        printf("%3.0f%% ", threshold);
        
        for (int x = 0; x < count; x++) {
            double val = values[x];
            int height = (int)((val * (GRAPH_HEIGHT - 1)) / max_val);
            printf("%s", height >= y ? "█" : " ");
        }
        printf("\n");
    }
    
    // Draw time axis
    printf("     ");
    for (int i = 0; i < 6; i++) {
        int x = (i * count) / 6;
        printf("%-10d", (int)(x * UPDATE_INTERVAL));
    }
    printf("\nTime (s)\n");
}

void display_resource_graphs(void) {
    static bool first_display = true;
    double cpu_values[MAX_RESOURCE_POINTS];
    double mem_values[MAX_RESOURCE_POINTS];
    double disk_values[MAX_RESOURCE_POINTS];
    
    int start_idx = (resource_history.current_index - resource_history.total_points + MAX_RESOURCE_POINTS) % MAX_RESOURCE_POINTS;
    
    for (int i = 0; i < resource_history.total_points; i++) {
        int idx = (start_idx + i) % MAX_RESOURCE_POINTS;
        cpu_values[i] = resource_history.points[idx].cpu_usage;
        mem_values[i] = resource_history.points[idx].memory_usage;
        disk_values[i] = resource_history.points[idx].disk_io;
    }

    if (first_display) {
        // On first display, clear screen and add padding
        printf("\033[2J");  // Clear entire screen
        printf("\033[H");   // Move cursor to top
        // Add some padding lines to preserve command history
        for (int i = 0; i < 35; i++) printf("\n");
        first_display = false;
    }
    
    // Move cursor up to the start of the monitoring area
    printf("\033[35A");  // Move up 35 lines
    printf("\033[K");    // Clear line
    
    printf("Resource Usage Monitor (Updated every %d second%s)\n", 
           UPDATE_INTERVAL, UPDATE_INTERVAL > 1 ? "s" : "");
    
    draw_graph("CPU Usage", cpu_values, resource_history.total_points);
    draw_graph("Memory Usage", mem_values, resource_history.total_points);
    draw_graph("Disk Usage", disk_values, resource_history.total_points);

    // Move cursor to the bottom of the monitoring area
    printf("\033[E");  // Move to beginning of next line
    
    fflush(stdout);
}

void track_command_execution(const char *command, double execution_time, bool had_error) {
    // Find existing command or create new entry
    int cmd_idx = -1;
    for (int i = 0; i < learning_stats.command_count; i++) {
        if (strcmp(learning_stats.commands[i].command, command) == 0) {
            cmd_idx = i;
            break;
        }
    }
    
    if (cmd_idx == -1 && learning_stats.command_count < MAX_TRACKED_COMMANDS) {
        cmd_idx = learning_stats.command_count++;
        strncpy(learning_stats.commands[cmd_idx].command, command, sizeof(learning_stats.commands[cmd_idx].command) - 1);
        learning_stats.commands[cmd_idx].first_use = time(NULL);
    }
    
    if (cmd_idx != -1) {
        CommandStats *stats = &learning_stats.commands[cmd_idx];
        stats->usage_count++;
        stats->last_use = time(NULL);
        if (had_error) stats->error_count++;
        stats->avg_execution_time = ((stats->avg_execution_time * (stats->usage_count - 1)) + execution_time) / stats->usage_count;
    }
    
    learning_stats.total_commands_executed++;
    if (had_error) learning_stats.total_errors++;
}

const char *get_proficiency_level(int usage_count, int error_rate) {
    if (usage_count < 5) return "Beginner";
    if (usage_count < 15) return "Intermediate";
    if (error_rate < 10) return "Advanced";
    return "Proficient";
}

void display_learning_dashboard(void) {
    time_t current_time = time(NULL);
    double session_duration = difftime(current_time, learning_stats.session_start);
    
    printf("\033[2J\033[H");  // Clear screen
    printf("Learning Analytics Dashboard\n");
    printf("==========================\n\n");
    
    printf("Session Statistics:\n");
    printf("- Duration: %.1f minutes\n", session_duration / 60.0);
    printf("- Total Commands: %d\n", learning_stats.total_commands_executed);
    printf("- Success Rate: %.1f%%\n\n", 
           100.0 * (1.0 - (double)learning_stats.total_errors / learning_stats.total_commands_executed));
    
    printf("Command Usage (Top 10):\n");
    printf("%-20s %-10s %-15s %-10s %-15s\n", 
           "Command", "Uses", "Errors", "Avg Time", "Proficiency");
    printf("------------------------------------------------------------\n");
    
    // Sort commands by usage count
    int indices[MAX_TRACKED_COMMANDS];
    for (int i = 0; i < learning_stats.command_count; i++) indices[i] = i;
    
    for (int i = 0; i < learning_stats.command_count - 1; i++) {
        for (int j = 0; j < learning_stats.command_count - i - 1; j++) {
            if (learning_stats.commands[indices[j]].usage_count < 
                learning_stats.commands[indices[j + 1]].usage_count) {
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
    
    // Display top 10 commands
    for (int i = 0; i < learning_stats.command_count && i < 10; i++) {
        CommandStats *stats = &learning_stats.commands[indices[i]];
        int error_rate = (int)(100.0 * stats->error_count / stats->usage_count);
        printf("%-20s %-10d %-15d %-10.2fs %-15s\n",
               stats->command, stats->usage_count, stats->error_count,
               stats->avg_execution_time,
               get_proficiency_level(stats->usage_count, error_rate));
    }
    
    generate_learning_suggestions();
}

void generate_learning_suggestions(void) {
    printf("\nLearning Suggestions:\n");
    printf("-------------------\n");
    
    // Find least used commands
    printf("1. Try exploring these commands more:\n");
    for (int i = 0; i < learning_stats.command_count; i++) {
        if (learning_stats.commands[i].usage_count < 5) {
            printf("   - %s (used %d times)\n", 
                   learning_stats.commands[i].command,
                   learning_stats.commands[i].usage_count);
        }
    }
    
    // Find commands with high error rates
    printf("\n2. Practice these commands to reduce errors:\n");
    for (int i = 0; i < learning_stats.command_count; i++) {
        double error_rate = (double)learning_stats.commands[i].error_count / 
                          learning_stats.commands[i].usage_count;
        if (error_rate > 0.2 && learning_stats.commands[i].usage_count > 5) {
            printf("   - %s (%.1f%% error rate)\n", 
                   learning_stats.commands[i].command,
                   error_rate * 100);
        }
    }
    
    printf("\nTip: Type 'help' to see all available commands and features.\n");
} 