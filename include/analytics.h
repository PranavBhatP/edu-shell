#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <time.h>
#include <stdbool.h>

#define MAX_RESOURCE_POINTS 60  // Store last 60 data points
#define MAX_TRACKED_COMMANDS 50
#define GRAPH_WIDTH 60
#define GRAPH_HEIGHT 8
#define UPDATE_INTERVAL 1  // Update every second

// Resource monitoring structures
typedef struct {
    time_t timestamp;
    double cpu_usage;
    double memory_usage;
    double disk_io;
} ResourcePoint;

typedef struct {
    ResourcePoint points[MAX_RESOURCE_POINTS];
    int current_index;
    int total_points;
} ResourceHistory;

// Learning analytics structures
typedef struct {
    char command[64];
    int usage_count;
    time_t first_use;
    time_t last_use;
    int error_count;
    double avg_execution_time;
} CommandStats;

typedef struct {
    CommandStats commands[MAX_TRACKED_COMMANDS];
    int command_count;
    int total_commands_executed;
    int total_errors;
    time_t session_start;
} LearningStats;

// Function prototypes
void initialize_analytics(void);
void cleanup_analytics(void);

// Resource monitoring functions
void update_resource_usage(void);
void display_resource_graphs(void);
double get_cpu_usage(void);
double get_memory_usage(void);
double get_disk_io(void);

// Learning analytics functions
void track_command_execution(const char *command, double execution_time, bool had_error);
void display_learning_dashboard(void);
void generate_learning_suggestions(void);
const char *get_proficiency_level(int usage_count, int error_rate);

#endif 