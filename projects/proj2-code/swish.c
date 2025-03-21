#define _GNU_SOURCE

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    struct sigaction sac;
    sac.sa_handler = SIG_IGN;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Initialize token vector and job list
    strvec_t tokens;
    strvec_init(&tokens);
    job_list_t jobs;
    job_list_init(&jobs);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Need to remove trailing '\n' from cmd. There are fancier ways.
        int i = 0;
        while (cmd[i] != '\n') {
            i++;
        }
        cmd[i] = '\0';

        // Tokenizing the input command
        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            job_list_free(&jobs);
            return 1;
        }
        // If no tokens were found, print prompt again
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }
        const char *first_token = strvec_get(&tokens, 0);

        if (strcmp(first_token, "pwd") == 0) {
            char cwd[CMD_LEN];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("Error printing directory");
            }
        }

        else if (strcmp(first_token, "cd") == 0) {
            if (tokens.length > 1) {
                if (chdir(strvec_get(&tokens, 1)) == -1) {
                    perror("chdir");
                }
            } else {
                // If no argument is provided, change to the home directory
                const char *home = getenv("HOME");
                if (home == NULL) {
                    fprintf(stderr, "HOME environment variable not set\n");
                } else if (chdir(home) == -1) {
                    perror("chdir");
                }
            }
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            break;
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            break;
        }

        // Task 5: Print out current list of pending jobs
        else if (strcmp(first_token, "jobs") == 0) {
            int i = 0;
            job_t *current = jobs.head;
            while (current != NULL) {
                char *status_desc;
                if (current->status == BACKGROUND) {
                    status_desc = "background";
                } else {
                    status_desc = "stopped";
                }
                printf("%d: %s (%s)\n", i, current->name, status_desc);
                i++;
                current = current->next;
            }
        }

        // Task 5: Move stopped job into foreground
        else if (strcmp(first_token, "fg") == 0) {
            if (resume_job(&tokens, &jobs, 1) == -1) {
                printf("Failed to resume job in foreground\n");
            }
        }

        // Task 6: Move stopped job into background
        else if (strcmp(first_token, "bg") == 0) {
            if (resume_job(&tokens, &jobs, 0) == -1) {
                printf("Failed to resume job in background\n");
            }
        }

        // Task 6: Wait for a specific job identified by its index in job list
        else if (strcmp(first_token, "wait-for") == 0) {
            if (await_background_job(&tokens, &jobs) == -1) {
                printf("Failed to wait for background job\n");
            }
        }

        // Task 6: Wait for all background jobs
        else if (strcmp(first_token, "wait-all") == 0) {
            if (await_all_background_jobs(&jobs) == -1) {
                printf("Failed to wait for all background jobs\n");
            }
        }

        else {
            // Forking a child process to run the command
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork failed");
                return 1;
            }

            // Task 4: Child Process Setup
            if (pid == 0) {
                // Creating a new process group for the child process
                if (setpgid(0, 0) == -1) {
                    perror("setpgid failed");
                    exit(1);
                }

                struct sigaction sa;
                sa.sa_handler = SIG_DFL;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;
                if (sigaction(SIGTTOU, &sa, NULL) == -1 || sigaction(SIGTTIN, &sa, NULL) == -1) {
                    perror("sigaction failed");
                    exit(1);
                }

                if (run_command(&tokens) == -1) {
                    exit(1);
                }
            }
            // parent process block
            else {
                int status;
                int is_background = 0;

                // Task 5: Check if the last token is "&" for background process
                if (tokens.length > 1 && strcmp(strvec_get(&tokens, tokens.length - 1), "&") == 0) {
                    is_background = 1;
                    strvec_take(&tokens, tokens.length - 1);    // Remove the "&"
                }

                // Task 4: Parent process handles foreground job and waits
                if (!is_background) {
                    if (tcsetpgrp(STDIN_FILENO, pid) == -1) {
                        perror("tcsetpgrp failed");
                        return -1;
                    }

                    if (waitpid(pid, &status, WUNTRACED) == -1) {
                        perror("waitpid failed");
                        return -1;
                    }

                    if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
                        perror("tcsetpgrp failed");
                        return -1;
                    }

                    if (WIFSTOPPED(status)) {
                        job_list_add(&jobs, pid, strvec_get(&tokens, 0), STOPPED);
                    }
                } else {
                    job_list_add(&jobs, pid, strvec_get(&tokens, 0), BACKGROUND);
                }
            }
        }

        // clear token vector for next command
        strvec_clear(&tokens);
        printf("%s", PROMPT);
    }

    // freeing job list memory before exiting
    job_list_free(&jobs);
    return 0;
}
