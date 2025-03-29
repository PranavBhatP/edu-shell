# EduShell - Educational Unix Shell

EduShell is an educational Unix shell implementation designed to help students learn shell concepts through hands-on experience. It combines the functionality of a basic shell with educational features like tutorials, command suggestions, and safe file operations.

## Features

### 1. Basic Shell Operations
- Command execution with PATH resolution
- Built-in commands (cd, pwd, ls, echo, etc.)
- Command history tracking
- Background process support using &
- Input/Output redirection (>, >>, <)

### 2. Educational Features
- Interactive tutorial mode (`tutorial` command)
  - Step-by-step guidance through basic shell concepts
  - Immediate feedback and hints
  - Progressive learning path from basic to advanced concepts
  - Practice exercises with validation

- Command auto-correction
  - Suggests correct commands when typos are made
  - Shows command usage information
  - Levenshtein distance algorithm for suggestions
  - Interactive confirmation (y/n) for suggestions

### 3. Safety Features
- Trash system for safe file deletion
  - Files are moved to ~/.edushell_trash instead of permanent deletion
  - Restore deleted files using `restore` command
  - List deleted files with `trash-list`
  - Timestamp-based file tracking

### 4. Script Support
- Execute shell scripts with .esh extension
- Support for comments and empty lines
- Line-by-line execution with error handling
- Script status reporting

### 5. Additional Features
- Command logging
- Error handling with descriptive messages
- Colorized output for better readability

## Usage
