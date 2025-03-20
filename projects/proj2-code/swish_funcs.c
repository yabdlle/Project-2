#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    // TODO Task 0: Tokenize string s
    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error

    if (s == NULL || tokens == NULL) {
        return -1;
    }
    char *tok = strtok(s, " ");    // Split string s into spaces
    while (tok != NULL) {
        if (strvec_add(tokens, tok) != 0) {    // Add token
            perror("Failed to add token");
            return -1;
        }
        tok = strtok(NULL, " ");    // Get the next token
    }

    return 0;
}

int run_command(strvec_t *tokens) {
    if (tokens == NULL || tokens->length == 0) {
        fprintf(stderr, "run_command: No command provided\n");
        return -1;
    }

    char *argv[MAX_ARGS];
    int i, in_fd = -1, out_fd = -1;
    int argc = 0;

    for (i = 0; i < tokens->length && argc < MAX_ARGS - 1; i++) {
        char *token = strvec_get(tokens, i);

        if (strcmp(token, "<") == 0) {
            if (i + 1 < tokens->length) {
                in_fd = open(strvec_get(tokens, i + 1), O_RDONLY);
                if (in_fd == -1) {
                    perror("Failed to open input file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else if (strcmp(token, ">") == 0) {
            if (i + 1 < tokens->length) {
                out_fd = open(strvec_get(tokens, i + 1), O_WRONLY | O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR);
                if (out_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else if (strcmp(token, ">>") == 0) {
            if (i + 1 < tokens->length) {
                out_fd = open(strvec_get(tokens, i + 1),
                              O_WRONLY | O_CREAT | O_APPEND | S_IRUSR | S_IWUSR);
                if (out_fd == -1) {
                    perror("Failed to open output file");
                    return -1;
                }
                i++;
            } else {
                fprintf(stderr, "Missing file");
                return -1;
            }
        } else {
            argv[argc++] = token;
        }
    }
    argv[argc] = NULL;

    if (in_fd != -1) {
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("dup2 input redirection failed");
            close(in_fd);
            return -1;
        }
        close(in_fd);
    }

    if (out_fd != -1) {
        if (dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output redirection failed");
            close(out_fd);
            return -1;
        }
        close(out_fd);
    }

    execvp(argv[0], argv);
    perror("exec");
    exit(EXIT_FAILURE);
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {

    // checking if the correct number of tokens is provided
    if (tokens->length < 2) {
        fprintf(stderr, "Missing job index.");
        return -1;
    }

    // parsing the job index from token 1
    int job_ind;
    if (sscanf(tokens->data[1], "%d", &job_ind) != 1) {
        fprintf(stderr, "Invalid job index\n");
        return -1;
    }

    // retrieving job from the job list
    job_t *job = job_list_get(jobs, job_ind);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    // getting process group ID of job
    pid_t job_pgid = job->pid;

    // resume job in the foreground
    if (is_foreground) {
        // I need to move job's process group to forground
        if (tcsetpgrp(STDERR_FILENO, job_pgid) == -1) {
            perror("tcsetpgrp failed.");
            return -1;
        }

        // sending the sigcontinue to job's process group
        if (kill(-job_pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
            return -1;
        }

        // now we wait for job to terminate or stop
        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1) {
            perror("waitpid failed.");
            return -1;
        }

        // if job has terminated check
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // remove job from job list if has terminated
            if (job_list_remove(jobs, job_ind) == -1) {
                fprintf(stderr, "failed to remove job from list\n");
                return -1;
            }
        }

        // restore shell's process group to foreground
        if (tcsetpgrp(STDERR_FILENO, getpgrp()) == -1) {
            perror("tcsetpgrp failed.");
            return -1;
        }
    }

    // resume job in background
    else {
        // sending sigcontinue to job's process group
        if (kill(-job_pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT) failed.");
            return -1;
        }

        // update the job's status to background
        job->status = BACKGROUND;
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {

    // Ensuring tokens and jobs pointers are valid
    if (tokens == NULL || jobs == NULL) {
        fprintf(stderr, "await_background_job: Invalid parameters (NULL pointer)\n");
        return -1;
    }

    // checking enough tokens
    if (tokens->length < 2) {
        fprintf(stderr, "Missing job index for await_background_job");
        return -1;
    }

    // parsing job index from second token
    int job_ind;
    if (sscanf(tokens->data[1], "%d", &job_ind) != 1) {
        fprintf(stderr, "Invalid job index\n");
        return -1;
    }

    // retrieving job from job list
    job_t *job = job_list_get(jobs, job_ind);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    // ensuring job is a background
    if (job->status != BACKGROUND) {
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }

    // now waiting for job to terminate or stop
    int status;
    if (waitpid(job->pid, &status, WUNTRACED) == -1) {
        perror("waitpid failed.");
        return -1;
    }

    // removing job from job list if it's terminated
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        if (job_list_remove(jobs, job_ind) == -1) {
            fprintf(stderr, "Failed to remove job from list\n");
            return -1;
        }
    }

    // updating job status to STOPPED if it was stopped.
    if (WIFSTOPPED(status)) {
        job->status = STOPPED;
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    job_t *current = jobs->head;

    while (current != NULL) {
        if (current->status == BACKGROUND) {
            int status;

            if (waitpid(current->pid, &status, WUNTRACED) == -1) {
                perror("waitpid failed.");
                return -1;
            }

            if (WIFSTOPPED(status)) {
                current->status = STOPPED;
            }

            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                current->status = STOPPED;
            }
        }
        current = current->next;
    }

    job_list_remove_by_status(jobs, STOPPED);
    job_list_remove_by_status(jobs, BACKGROUND);

    return 0;
}
