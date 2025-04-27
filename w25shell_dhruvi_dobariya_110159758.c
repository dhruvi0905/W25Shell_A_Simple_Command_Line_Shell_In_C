
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>


#define MAX_INPUT 1024
#define MAX_ARGS 6
#define MAX_CMDS 6
#define DELIM " \t\r\n\a"
#define MAX_LINE 256
#define CMDLINE_PATH "/proc/self/cmdline"

// Declare function to get the process's own name dynamically.
void get_process_name(char *name, size_t size);
// Declare function to execute a single command with arguments.
void execute_command(char **args);
// Declare function to execute a pipeline of commands.
void execute_pipe(char ***commands, int num_cmds);
// Declare function to execute a reverse pipeline of commands.
void execute_reverse_pipe(char ***commands, int num_cmds);
// Declare function to append contents of two files to each other.
void append_files(char *file1, char *file2);
// Declare function to count words in a file.
void count_words(char *file);
// Declare function to concatenate contents of multiple files.
void concatenate_files(char **files, int num_files);
// Declare function to handle input/output redirection for a command.
void redirect_command(char **args, char *input_file, char *output_file, int append);
// Declare function to execute commands sequentially.
void sequential_execution(char ***commands, int num_cmds);
// Declare function to execute commands conditionally with && or || operators.
void conditional_execution(char ***commands, int num_cmds, char **ops);
// Declare function to kill the current terminal process.
void kill_current_terminal();
// Declare function to kill all terminals with the same process name.
void kill_all_terminals(char *self_name);
// Declare function to parse input into commands and operators.
int parse_input(char *line, char **commands, char **operators, const char *delimiter);
// Declare function to split a command string into arguments.
char **split_command(char *command);

// Function to trim leading and trailing whitespace from a string.
char *trim_whitespace(char *str) {
    // Declare a pointer to point to the end of the string.
    char *end;
    // Move str pointer past leading spaces or tabs.
    while (*str == ' ' || *str == '\t') str++;
    // Set end pointer to the last character of the string.
    end = str + strlen(str) - 1;
    // Move end pointer back past trailing spaces or tabs.
    while (end > str && (*end == ' ' || *end == '\t')) end--;
    // Null-terminate the string after the last non-whitespace character.
    *(end + 1) = '\0';
    // Return the trimmed string.
    return str;
}

// Function to retrieve the process's own name from /proc/self/cmdline.
void get_process_name(char *name, size_t size) {
    // Open the /proc/self/cmdline file in read mode.
    FILE *fp = fopen(CMDLINE_PATH, "r");
    // Check if the file was successfully opened.
    if (fp) {
        // Read up to size-1 bytes from the file into the name buffer.
        fread(name, 1, size - 1, fp);
        // Close the file.
        fclose(fp);
        // Find the last occurrence of '/' to get the basename of the executable.
        char *basename = strrchr(name, '/');
        // If a basename is found, copy it into name (excluding the path).
        if (basename) {
            // Copy the basename (everything after the last '/') into name.
            strcpy(name, basename + 1);
        }
        // Ensure the name is null-terminated by setting the last byte to '\0'.
        name[size - 1] = '\0';
    } else {
        // Print an error message if the file couldn't be opened.
        perror("Failed to get process name");
        // Fallback to a default name "w25shell" if retrieval fails.
        strcpy(name, "w25shell");
    }
}

// Main function - entry point of the program.
int main() {
    // Declare a buffer to store user input, with size MAX_INPUT.
    char input[MAX_INPUT];
    // Declare a buffer to store the process name, with size MAX_LINE.
    char self_name[MAX_LINE];
    
    // Call get_process_name to dynamically retrieve the current process name into self_name.
    get_process_name(self_name, sizeof(self_name));

    // Start an infinite loop to continuously prompt for and process user input.
    while (1) {
        // Print the shell prompt using the dynamic process name (e.g., "w25shell$ " or "a3$ ").
        printf("%s$ ", self_name);
        // Flush stdout to ensure the prompt is displayed immediately.
        fflush(stdout);

        // Read user input into the input buffer from stdin, exit on failure (e.g., EOF).
        if (!fgets(input, MAX_INPUT, stdin)) {
            // Print a newline for clean exit formatting.
            printf("\n");
            // Break the loop to exit the program (e.g., on Ctrl+D).
            break;
        }

        // Remove the trailing newline from the input by replacing it with a null terminator.
        input[strcspn(input, "\n")] = 0;

        // Skip to the next loop iteration if the input is empty.
        if (strlen(input) == 0)
            continue;

        // Check if the input is the "killterm" command to terminate the current terminal.
        if (strcmp(input, "killterm") == 0) {
            // Call the function to kill the current terminal process.
            kill_current_terminal();
        // Check if the input is the "killallterms" command to terminate all matching terminals.
        } else if (strcmp(input, "killallterms") == 0) {
            // Call the function to kill all processes with the same name as self_name.
            kill_all_terminals(self_name);
        // Check if the input starts with '#' to count words in a file.
        } else if (input[0] == '#' && input[1] == ' ') {
            // Extract the filename by skipping the "# " prefix.
            char *file = input + 2;
            // Call the function to count and print the number of words in the file.
            count_words(file);
        // Check if the input contains " ~ " to append two files to each other.
        } else if (strstr(input, " ~ ")) {
            // Extract the first filename by tokenizing on " ~".
            char *file1 = strtok(input, " ~");
            // Extract the second filename by continuing tokenization.
            char *file2 = strtok(NULL, " ~");
            // Check if both filenames were successfully extracted.
            if (file1 && file2)
                // Call the function to append the contents of file1 and file2 to each other.
                append_files(file1, file2);
            else
                // Print an error message if the syntax is incorrect.
                fprintf(stderr, "Error: ~ requires two .txt files\n");
        // Check if the input contains '+' to concatenate multiple files.
        } else if (strchr(input, '+')) {
            // Declare an array to store the commands (filenames in this case).
            char *commands[MAX_CMDS];
            // Declare an array to store operators (not used for '+' but required by parse_input).
            char *operators[MAX_CMDS - 1];
            // Parse the input into commands using '+' as the delimiter.
            int num_files = parse_input(input, commands, NULL, "+");

            // Check if the number of files is between 2 and 5 (inclusive).
            if (num_files > 1 && num_files <= 5)
                // Call the function to concatenate the files and print their contents.
                concatenate_files(commands, num_files);
            else
                // Print an error message if the number of files is invalid.
                fprintf(stderr, "Error: + requires 2 to 5 files\n");

            // Free the memory allocated for each command (filename) in the commands array.
            for (int i = 0; i < num_files; i++) free(commands[i]);
        // Check if the input contains ';' to execute commands sequentially.
        } else if (strchr(input, ';')) {
            // Declare an array to store the commands.
            char *commands[MAX_CMDS];
            // Parse the input into commands using ';' as the delimiter.
            int num_cmds = parse_input(input, commands, NULL, ";");

            // Check if the number of commands is between 1 and 4 (inclusive).
            if (num_cmds >= 1 && num_cmds <= 4) {
                // Declare an array to store the tokenized arguments for each command.
                char **cmd_array[num_cmds];
                // Loop through each command to split it into arguments.
                for (int i = 0; i < num_cmds; i++)
                    // Split the command into arguments and store in cmd_array.
                    cmd_array[i] = split_command(commands[i]);

                // Call the function to execute the commands sequentially.
                sequential_execution(cmd_array, num_cmds);

                // Free the memory allocated for each command's arguments.
                for (int i = 0; i < num_cmds; i++) {
                    // Free each argument in the command.
                    for (int j = 0; cmd_array[i][j]; j++) free(cmd_array[i][j]);
                    // Free the array of arguments for the command.
                    free(cmd_array[i]);
                }
            } else {
                // Print an error message if the number of commands is invalid.
                fprintf(stderr, "Error: ; supports up to 4 commands\n");
            }
            // Free the memory allocated for each command string.
            for (int i = 0; i < num_cmds; i++) free(commands[i]);
        // Check if the input contains "&&" or "||" for conditional execution.
        } else if (strstr(input, "&&") || strstr(input, "||")) {
            // Declare an array to store the commands.
            char *commands[MAX_CMDS];
            // Declare an array to store the operators (&& or ||).
            char *operators[MAX_CMDS - 1];
            // Initialize the operators array to all zeros (NULL pointers).
            memset(operators, 0, sizeof(operators));
            // Parse the input into commands and operators.
            int num_cmds = parse_input(input, commands, operators, NULL);

            // Check if the number of commands is between 1 and 5 (inclusive).
            if (num_cmds >= 1 && num_cmds <= 5) {
                // Declare an array to store the tokenized arguments for each command.
                char **cmd_array[num_cmds];
                // Initialize a flag to track if all commands are valid.
                int valid = 1;
                // Loop through each command to validate and split it.
                for (int i = 0; i < num_cmds; i++) {
                    // Split the command into arguments and store in cmd_array.
                    cmd_array[i] = split_command(commands[i]);
                    // Initialize a counter for the number of arguments.
                    int argc = 0;
                    // Count the number of arguments in the command.
                    while (cmd_array[i][argc]) argc++;
                    // Check if the number of arguments is between 1 and 5.
                    if (argc < 1 || argc > 5) {
                        // Print an error message if the argument count is invalid.
                        fprintf(stderr, "Error: Command %d has invalid argc (%d), must be 1-5\n", i + 1, argc);
                        // Set the valid flag to 0 to skip execution.
                        valid = 0;
                        // Break the loop since validation failed.
                        break;
                    }
                }
                // Check if all commands are valid.
                if (valid) {
                    // Call the function to execute the commands conditionally.
                    conditional_execution(cmd_array, num_cmds, operators);
                    // Free the memory allocated for each command's arguments.
                    for (int i = 0; i < num_cmds; i++) {
                        // Free each argument in the command.
                        for (int j = 0; cmd_array[i][j]; j++) free(cmd_array[i][j]);
                        // Free the array of arguments for the command.
                        free(cmd_array[i]);
                    }
                }
            } else {
                // Print an error message if the number of commands is invalid.
                fprintf(stderr, "Error: Conditional execution supports up to 5 commands\n");
            }
            // Free the memory allocated for each command string.
            for (int i = 0; i < num_cmds; i++) free(commands[i]);
            // Free the memory allocated for each operator string.
            for (int i = 0; i < num_cmds - 1; i++) if (operators[i]) free(operators[i]);
        // Check if the input contains '|' (but not '=') for piping commands.
        } else if (strchr(input, '|') && !strchr(input, '=')) {
            // Declare an array to store the commands.
            char *commands[MAX_CMDS];
            // Parse the input into commands using '|' as the delimiter.
            int num_cmds = parse_input(input, commands, NULL, "|");

            // Check if the number of commands is between 1 and 5 (inclusive).
            if (num_cmds >= 1 && num_cmds <= 5) {
                // Declare an array to store the tokenized arguments for each command.
                char **cmd_array[num_cmds];
                // Loop through each command to split it into arguments.
                for (int i = 0; i < num_cmds; i++)
                    // Split the command into arguments and store in cmd_array.
                    cmd_array[i] = split_command(commands[i]);

                // Call the function to execute the commands in a pipeline.
                execute_pipe(cmd_array, num_cmds);

                // Free the memory allocated for each command's arguments.
                for (int i = 0; i < num_cmds; i++) {
                    // Free each argument in the command.
                    for (int j = 0; cmd_array[i][j]; j++) free(cmd_array[i][j]);
                    // Free the array of arguments for the command.
                    free(cmd_array[i]);
                }
            } else {
                // Print an error message if the number of commands is invalid.
                fprintf(stderr, "Error: Pipe supports up to 5 commands\n");
            }
            // Free the memory allocated for each command string.
            for (int i = 0; i < num_cmds; i++) free(commands[i]);
        // Check if the input contains '=' for reverse piping.
        } else if (strchr(input, '=')) {
            // Declare an array to store the commands.
            char *commands[MAX_CMDS];
            // Parse the input into commands using '=' as the delimiter.
            int num_cmds = parse_input(input, commands, NULL, "=");

            // Check if the number of commands is between 1 and 5 (inclusive).
            if (num_cmds >= 1 && num_cmds <= 5) {
                // Declare an array to store the tokenized arguments for each command.
                char **cmd_array[num_cmds];
                // Loop through each command to split it into arguments.
                for (int i = 0; i < num_cmds; i++)
                    // Split the command into arguments and store in cmd_array.
                    cmd_array[i] = split_command(commands[i]);

                // Call the function to execute the commands in a reverse pipeline.
                execute_reverse_pipe(cmd_array, num_cmds);

                // Free the memory allocated for each command's arguments.
                for (int i = 0; i < num_cmds; i++) {
                    // Free each argument in the command.
                    for (int j = 0; cmd_array[i][j]; j++) free(cmd_array[i][j]);
                    // Free the array of arguments for the command.
                    free(cmd_array[i]);
                }
            } else {
                // Print an error message if the number of commands is invalid.
                fprintf(stderr, "Error: Reverse pipe supports up to 5 commands\n");
            }
            // Free the memory allocated for each command string.
            for (int i = 0; i < num_cmds; i++) free(commands[i]);
        // Check if the input contains '<' or '>' for redirection.
        } else if (strchr(input, '<') || strchr(input, '>')) {
            // Check if there is an input redirection ('<').
            int is_input_redirect = strchr(input, '<') != NULL;
            // Check if there is an output append redirection ('>>').
            int is_output_append = strstr(input, ">>") != NULL;
            // Check if there is an output overwrite redirection ('>' but not '>>').
            int is_output_redirect = strchr(input, '>') != NULL && !is_output_append;

            // Initialize pointers for the command part and file part of the input.
            char *command_part = NULL;
            char *file_part = NULL;

            // If output append redirection is present, split on ">>".
            if (is_output_append) {
                // Extract the command part before ">>".
                command_part = strtok(input, ">>");
                // Extract the file part after ">>".
                file_part = strtok(NULL, ">>");
            // If output overwrite redirection is present, split on ">".
            } else if (is_output_redirect) {
                // Extract the command part before ">".
                command_part = strtok(input, ">");
                // Extract the file part after ">".
                file_part = strtok(NULL, ">");
            // If input redirection is present, split on "<".
            } else if (is_input_redirect) {
                // Extract the command part before "<".
                command_part = strtok(input, "<");
                // Extract the file part after "<".
                file_part = strtok(NULL, "<");
            }

            // Check if both the command part and file part were successfully extracted.
            if (command_part && file_part) {
                // Split the command part into arguments.
                char **cmd = split_command(command_part);

                // If input redirection is present, redirect input from the file.
                if (is_input_redirect) {
                    // Call redirect_command with input file and no output file.
                    redirect_command(cmd, file_part, NULL, 0);
                // If output append redirection is present, append output to the file.
                } else if (is_output_append) {
                    // Call redirect_command with no input file and append mode.
                    redirect_command(cmd, NULL, file_part, 1);
                // If output overwrite redirection is present, overwrite the file.
                } else if (is_output_redirect) {
                    // Call redirect_command with no input file and overwrite mode.
                    redirect_command(cmd, NULL, file_part, 0);
                }

                // Free the memory allocated for each argument in the command.
                for (int i = 0; cmd[i] != NULL; i++) {
                    // Free the memory for the current argument.
                    free(cmd[i]);
                }
                // Free the array of arguments.
                free(cmd);
            } else {
                // Print an error message if the redirection syntax is invalid.
                fprintf(stderr, "Error: Redirection requires a valid command and file\n");
            }
        // Default case: treat the input as a regular command.
        } else {
            // Split the input into arguments.
            char **args = split_command(input);

            // Initialize a counter for the number of arguments.
            int argc = 0;
            // Count the number of arguments.
            while (args[argc]) argc++;

            // Check if the number of arguments is between 1 and 5.
            if (argc >= 1 && argc <= 5)
                // Execute the command with its arguments.
                execute_command(args);
            else
                // Print an error message if the argument count is invalid.
                fprintf(stderr, "Error: Command argument count must be 1-5\n");

            // Free the memory allocated for each argument.
            for (int i = 0; i < argc; i++) {
                // Free the memory for the current argument.
                free(args[i]);
            }
            // Free the array of arguments.
            free(args);
        }
    }

    // Return 0 to indicate successful program completion.
    return 0;
}

// Function to execute a single command with its arguments.
void execute_command(char **args) {
    // Fork a new process to execute the command.
    pid_t pid = fork();

    // Check if this is the child process (pid == 0).
    if (pid == 0) {
        // Execute the command using execvp with the provided arguments.
        execvp(args[0], args);
        // If execvp fails, print an error message.
        perror("execvp failed");
        // Exit the child process with a failure status.
        exit(EXIT_FAILURE);
    // Check if this is the parent process (pid > 0).
    } else if (pid > 0) {
        // Declare a variable to store the child's exit status.
        int status;
        // Wait for the child process to complete.
        waitpid(pid, &status, 0);
    // If fork failed (pid < 0), print an error message.
    } else {
        // Print an error message if forking failed.
        perror("fork failed");
    }
}

// Function to kill the current terminal process.
void kill_current_terminal() {
    // Print a message indicating that the current terminal is being killed.
    printf("Killing current terminal...\n");
    // Exit the process with status 0.
    exit(0);
}

// Function to kill all terminals with the same process name as self_name.
void kill_all_terminals(char *self_name) {
    // Print a message indicating that all terminals with the given name are being killed.
    printf("Killing all %s terminals for your user...\n", self_name);
    // Flush stdout to ensure the message is printed immediately.
    fflush(stdout);

    // Get the PID of the current process.
    pid_t self_pid = getpid();
    // Initialize a flag to track if the current process should be killed.
    int self_killed = 0;
    // Declare a variable to control the number of passes through the process list.
    int pass;

    // Open the /proc directory to scan all processes.
    DIR *dir = opendir("/proc");
    // Check if the /proc directory was successfully opened.
    if (!dir) {
        // Print an error message if /proc couldn't be opened.
        perror("Failed to open /proc");
        // Return from the function since we can't proceed.
        return;
    }

    // Perform two passes to ensure all processes are killed (first pass skips self).
    for (pass = 0; pass < 2; pass++) {
        // Declare a pointer to store directory entries.
        struct dirent *entry;
        // Rewind the directory stream to start from the beginning for this pass.
        rewinddir(dir);

        // Read each entry in the /proc directory.
        while ((entry = readdir(dir)) != NULL) {
            // Check if the entry is a directory and its name is a number (PID).
            if (entry->d_type != DT_DIR || !isdigit(*entry->d_name)) {
                // Skip entries that are not directories or not numeric (not PIDs).
                continue;
            }

            // Convert the directory name (PID string) to an integer.
            int pid = atoi(entry->d_name);
            // Skip the current process in the first pass.
            if (pid == self_pid && pass == 0) {
                // Continue to the next entry since we don't want to kill ourselves yet.
                continue;
            }

            // Construct the path to the process's cmdline file (e.g., /proc/<pid>/cmdline).
            char cmdline_path[256];
            // Use snprintf to safely format the path string.
            snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);

            // Open the cmdline file for the process in read mode.
            FILE *fp = fopen(cmdline_path, "r");
            // Check if the file was successfully opened.
            if (!fp) {
                // Skip this process if we can't read its cmdline file.
                continue;
            }

            // Initialize a buffer to store the process name, filled with zeros.
            char proc_name[MAX_LINE] = {0};
            // Read the cmdline file into the proc_name buffer.
            fread(proc_name, 1, sizeof(proc_name) - 1, fp);
            // Close the cmdline file.
            fclose(fp);

            // Find the last occurrence of '/' to get the basename of the executable.
            char *basename = strrchr(proc_name, '/');
            // If a basename is found, copy it into proc_name (excluding the path).
            if (basename) {
                // Copy the basename (everything after the last '/') into proc_name.
                strcpy(proc_name, basename + 1);
            }

            // Skip processes with an empty name or named "bash".
            if (strlen(proc_name) == 0 || strcmp(proc_name, "bash") == 0) {
                // Continue to the next entry if the process name is empty or "bash".
                continue;
            }

            // Compare the process name with self_name to find matching processes.
            if (strcmp(proc_name, self_name) == 0) {
                // Check if this is the current process.
                if (pid == self_pid) {
                    // Mark that we need to kill the current process in the last pass.
                    self_killed = 1;
                } else {
                    // Print a message indicating that we're killing this process.
                    printf("Killing process: %d (%s)\n", pid, proc_name);
                    // Send a SIGKILL signal to terminate the process.
                    kill(pid, SIGKILL);
                    // Sleep for 10 milliseconds to allow the kill to take effect.
                    usleep(10000);
                }
            }
        }
    }

    // Close the /proc directory.
    closedir(dir);

    // Check if the current process was marked to be killed.
    if (self_killed) {
        // Print a message indicating that we're killing the current process.
        printf("Killing self: %d (%s)\n", self_pid, self_name);
        // Send a SIGKILL signal to terminate the current process.
        kill(self_pid, SIGKILL);
    }
}

// Function to count the number of words in a file.
void count_words(char *file) {
    // Open the specified file in read mode.
    FILE *fp = fopen(file, "r");
    // Check if the file was successfully opened.
    if (!fp) {
        // Print an error message if the file couldn't be opened.
        perror("Error opening file");
        // Return from the function since we can't proceed.
        return;
    }

    // Initialize a counter for the number of words.
    int count = 0;
    // Declare a buffer to store each word, with a maximum size of 256 characters.
    char word[256];
    // Read each word from the file using fscanf until there are no more words.
    while (fscanf(fp, "%255s", word) == 1)
        // Increment the word counter for each word read.
        count++;

    // Close the file.
    fclose(fp);
    // Print the total number of words in the file.
    printf("Number of words in %s: %d\n", file, count);
}

// Function to append the contents of two files to each other.
void append_files(char *file1, char *file2) {
    // Open the first file in read-write mode.
    FILE *fp1 = fopen(file1, "r+");
    // Open the second file in read-write mode.
    FILE *fp2 = fopen(file2, "r+");

    // Check if both files were successfully opened.
    if (!fp1 || !fp2) {
        // Print an error message if either file couldn't be opened.
        perror("Error opening files");
        // Close the first file if it was opened.
        if (fp1) fclose(fp1);
        // Close the second file if it was opened.
        if (fp2) fclose(fp2);
        // Return from the function since we can't proceed.
        return;
    }

    // Move the file pointer of fp2 to the beginning of file2.
    fseek(fp2, 0, SEEK_SET);
    // Move the file pointer of fp1 to the end of file1.
    fseek(fp1, 0, SEEK_END);
    // Declare a variable to store each character read.
    char ch;
    // Read each character from file2 until the end of file is reached.
    while ((ch = fgetc(fp2)) != EOF)
        // Write the character to file1, appending file2's contents to file1.
        fputc(ch, fp1);

    // Move the file pointer of fp1 back to the beginning of file1.
    fseek(fp1, 0, SEEK_SET);
    // Move the file pointer of fp2 to the end of file2.
    fseek(fp2, 0, SEEK_END);
    // Read each character from file1 until the end of file is reached.
    while ((ch = fgetc(fp1)) != EOF)
        // Write the character to file2, appending file1's contents to file2.
        fputc(ch, fp2);

    // Close the first file.
    fclose(fp1);
    // Close the second file.
    fclose(fp2);
    // Print a success message indicating the files were appended.
    printf("Appended contents of %s <-> %s successfully.\n", file1, file2);
}

// Function to concatenate the contents of multiple files and print them.
void concatenate_files(char **files, int num_files) {
    // Declare a buffer to store each line read from the files, with size 1024.
    char buffer[1024];

    // Loop through each file in the files array.
    for (int i = 0; i < num_files; i++) {
        // Trim whitespace from the filename.
        char *filename = trim_whitespace(files[i]);
        // Open the file in read mode.
        FILE *fp = fopen(filename, "r");
        // Check if the file was successfully opened.
        if (!fp) {
            // Print an error message if the file couldn't be opened.
            fprintf(stderr, "Error opening file: %s\n", filename);
            // Continue to the next file.
            continue;
        }

        // Read each line from the file until there are no more lines.
        while (fgets(buffer, sizeof(buffer), fp))
            // Print the line to stdout.
            printf("%s", buffer);

        // Close the file.
        fclose(fp);
    }

    // Print a message indicating that file concatenation is complete.
    printf("\nFile concatenation complete.\n");
}

// Function to execute commands sequentially.
void sequential_execution(char ***commands, int num_cmds) {
    // Loop through each command in the commands array.
    for (int i = 0; i < num_cmds; i++) {
        // Execute the current command with its arguments.
        execute_command(commands[i]);
    }
}

// Function to execute commands conditionally using && and || operators.
void conditional_execution(char ***commands, int num_cmds, char **ops) {
    // Initialize a variable to store the exit status of child processes.
    int status = 0;
    // Initialize a flag to track the result of the last command (1 for success).
    int last_result = 1;

    // Loop through each command in the commands array.
    for (int i = 0; i < num_cmds; i++) {
        // Check if we should skip this command based on previous results and operators.
        if (i > 0) {
            // If the previous operator was || and the last command succeeded, skip this command.
            if (strcmp(ops[i - 1], "||") == 0 && last_result) {
                // Continue to the next command.
                continue;
            }
            // If the previous operator was && and the last command failed, stop execution.
            if (strcmp(ops[i - 1], "&&") == 0 && !last_result) {
                // Break the loop to stop execution.
                break;
            }
        }

        // Fork a new process to execute the command.
        pid_t pid = fork();
        // Check if this is the child process (pid == 0).
        if (pid == 0) {
            // Execute the command using execvp with the provided arguments.
            execvp(commands[i][0], commands[i]);
            // If execvp fails, print an error message.
            perror("execvp failed");
            // Exit the child process with a failure status.
            exit(EXIT_FAILURE);
        // Check if this is the parent process (pid > 0).
        } else if (pid > 0) {
            // Wait for the child process to complete and get its exit status.
            waitpid(pid, &status, 0);
            // Update last_result based on whether the child exited normally with status 0.
            last_result = (WIFEXITED(status) && WEXITSTATUS(status) == 0);
        // If fork failed (pid < 0), print an error message.
        } else {
            // Print an error message if forking failed.
            perror("fork failed");
            // Break the loop since we can't proceed.
            break;
        }
    }
}

// Function to execute a pipeline of commands.
void execute_pipe(char ***commands, int num_cmds) {
    // Declare an array to store pipe file descriptors (2 FDs per pipe).
    int pipefd[2 * (num_cmds - 1)];
    // Declare an array to store the PIDs of child processes.
    pid_t pids[num_cmds];

    // Create pipes for communication between commands (num_cmds - 1 pipes needed).
    for (int i = 0; i < num_cmds - 1; i++) {
        // Create a pipe and store the file descriptors in pipefd.
        if (pipe(pipefd + i * 2) < 0) {
            // Print an error message if pipe creation failed.
            perror("Pipe creation failed");
            // Exit the program with a failure status.
            exit(EXIT_FAILURE);
        }
    }

    // Loop through each command to execute it in a child process.
    for (int i = 0; i < num_cmds; i++) {
        // Fork a new process for the current command.
        pids[i] = fork();

        // Check if this is the child process (pids[i] == 0).
        if (pids[i] == 0) {
            // If not the first command, redirect stdin from the previous pipe's read end.
            if (i > 0)
                // Duplicate the read end of the previous pipe to stdin.
                dup2(pipefd[(i - 1) * 2], STDIN_FILENO);

            // If not the last command, redirect stdout to the current pipe's write end.
            if (i < num_cmds - 1)
                // Duplicate the write end of the current pipe to stdout.
                dup2(pipefd[i * 2 + 1], STDOUT_FILENO);

            // Close all pipe file descriptors in the child process.
            for (int j = 0; j < 2 * (num_cmds - 1); j++)
                // Close the pipe file descriptor.
                close(pipefd[j]);

            // Execute the command using execvp with the provided arguments.
            execvp(commands[i][0], commands[i]);
            // If execvp fails, print an error message.
            perror("execvp failed");
            // Exit the child process with a failure status.
            exit(EXIT_FAILURE);
        }
    }

    // Close all pipe file descriptors in the parent process.
    for (int i = 0; i < 2 * (num_cmds - 1); i++)
        // Close the pipe file descriptor.
        close(pipefd[i]);

    // Wait for all child processes to complete.
    for (int i = 0; i < num_cmds; i++)
        // Wait for the child process with the given PID.
        waitpid(pids[i], NULL, 0);
}

// Function to execute a reverse pipeline of commands.
void execute_reverse_pipe(char ***commands, int num_cmds) {
    // Declare an array to store the commands in reverse order.
    char **reversed[num_cmds];
    // Reverse the order of the commands.
    for (int i = 0; i < num_cmds; i++)
        // Assign the commands in reverse order to the reversed array.
        reversed[i] = commands[num_cmds - i - 1];

    // Call execute_pipe with the reversed commands.
    execute_pipe(reversed, num_cmds);
}

// Function to handle input/output redirection for a command.
void redirect_command(char **args, char *input_file, char *output_file, int append) {
    // Fork a new process to execute the command.
    pid_t pid = fork();

    // Check if this is the child process (pid == 0).
    if (pid == 0) {
        // Check if an input file is provided for redirection.
        if (input_file) {
            // Open the input file in read-only mode.
            int fd_in = open(input_file, O_RDONLY);
            // Check if the input file was successfully opened.
            if (fd_in < 0) {
                // Print an error message if the file couldn't be opened.
                perror("Failed to open input file");
                // Exit the child process with a failure status.
                exit(EXIT_FAILURE);
            }
            // Redirect stdin to the input file.
            dup2(fd_in, STDIN_FILENO);
            // Close the input file descriptor since it's duplicated.
            close(fd_in);
        }

        // Check if an output file is provided for redirection.
        if (output_file) {
            // Set the file flags for opening: write-only, create if not exists, append or truncate.
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            // Open the output file with the specified flags and permissions 0644.
            int fd_out = open(output_file, flags, 0644);
            // Check if the output file was successfully opened.
            if (fd_out < 0) {
                // Print an error message if the file couldn't be opened.
                perror("Failed to open output file");
                // Exit the child process with a failure status.
                exit(EXIT_FAILURE);
            }
            // Redirect stdout to the output file.
            dup2(fd_out, STDOUT_FILENO);
            // Close the output file descriptor since it's duplicated.
            close(fd_out);
        }

        // Execute the command using execvp with the provided arguments.
        execvp(args[0], args);
        // If execvp fails, print an error message.
        perror("execvp failed");
        // Exit the child process with a failure status.
        exit(EXIT_FAILURE);
    // Check if this is the parent process (pid > 0).
    } else if (pid > 0) {
        // Declare a variable to store the child's exit status.
        int status;
        // Wait for the child process to complete.
        waitpid(pid, &status, 0);
    // If fork failed (pid < 0), print an error message.
    } else {
        // Print an error message if forking failed.
        perror("fork failed");
    }
}

// Function to parse the input line into commands and operators.
int parse_input(char *line, char **commands, char **operators, const char *delimiter) {
    // Initialize a counter for the number of commands.
    int count = 0;
    // Declare a pointer to store the strtok_r context.
    char *saveptr;

    // Check if a delimiter is provided (e.g., "+", ";", "|", "=").
    if (delimiter) {
        // Tokenize the line using the specified delimiter.
        char *token = strtok_r(line, delimiter, &saveptr);
        // Loop through each token until there are no more or we reach MAX_CMDS.
        while (token && count < MAX_CMDS) {
            // Store the trimmed token in the commands array.
            commands[count++] = strdup(trim_whitespace(token));
            // Get the next token.
            token = strtok_r(NULL, delimiter, &saveptr);
        }
    } else {
        // For conditional execution (&& and ||), tokenize on whitespace.
        char *token = strtok_r(line, " \t", &saveptr);
        // Loop through each token until there are no more or we reach MAX_CMDS.
        while (token && count < MAX_CMDS) {
            // Check if the token is "&&" or "||".
            if (strcmp(token, "&&") == 0 || strcmp(token, "||") == 0) {
                // If operators array is provided and we have at least one command, store the operator.
                if (operators && count > 0)
                    // Store the operator in the operators array.
                    operators[count - 1] = strdup(token);
                // Get the next token.
                token = strtok_r(NULL, " \t", &saveptr);
                // Continue to the next iteration.
                continue;
            }
            // Store the token in the commands array.
            commands[count++] = strdup(token);
            // Get the next token.
            token = strtok_r(NULL, " \t", &saveptr);
        }
    }

    // Return the number of commands parsed.
    return count;
}

// Function to split a command string into arguments.
char **split_command(char *command) {
    // Allocate memory for an array of argument pointers.
    char **args = malloc(MAX_ARGS * sizeof(char *));
    // Initialize a counter for the number of arguments.
    int count = 0;

    // Create a copy of the command string to avoid modifying the original.
    char *cmd_copy = strdup(command);
    // Tokenize the command string using the defined delimiters (DELIM).
    char *token = strtok(cmd_copy, DELIM);

    // Loop through each token until there are no more or we reach MAX_ARGS - 1.
    while (token && count < MAX_ARGS - 1) {
        // Store the token in the args array.
        args[count++] = strdup(token);
        // Get the next token.
        token = strtok(NULL, DELIM);
    }

    // Set the last element of args to NULL to terminate the argument list.
    args[count] = NULL;
    // Free the memory allocated for the command copy.
    free(cmd_copy);
    // Return the array of arguments.
    return args;
}