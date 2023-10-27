#include <linux/sched.h>    /* Definition of struct clone_args */
#include <sched.h>          /* Definition of CLONE_* constants */
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <signal.h>         /* Definition of SIGCHLD */
#include <unistd.h>
#include <error.h>          /* Definition of error_at_line */
#include <stdlib.h>         /* Definition of EXIT_FAILURE */
#include <errno.h>
#include <stdio.h>          /* Definition of fprintf */
#include <limits.h>         /* Definition of INT_MIN, INT_MAX */
#include <stdbool.h>        /* Definition of bool */
#include <stdint.h>         /* Definition of uintptr_t */

void help(FILE *stream, const char *progname)
{
    fprintf(stream, "%s PID [COMMAND [ARGS...]]\n"
                    "\tPID Target PID\n"
                    "\tCOMMAND [ARGS...] Command to execute\n", progname);
}

pid_t parse_pid(const char *pid_text, const char *progname)
{
    bool pid_parsed = false;
    char *pid_text_end = NULL;
    long target_pid = strtol(pid_text, &pid_text_end, 0);

    if (*pid_text_end != '\0')
        fprintf(stderr, "Unable to parse PID: %s\n", pid_text);
    else if (errno == ERANGE)
        fprintf(stderr, "PID out of range for long int: %s\n", pid_text);
    else if (target_pid < INT_MIN || target_pid > INT_MAX)
        fprintf(stderr, "PID out of range for pid_t: %s\n", pid_text);
    else
        pid_parsed = true;

    if (!pid_parsed) {
        help(stderr, progname);
        exit(EXIT_FAILURE);
    }

    return target_pid;
}

#define PID_ARGV_INDEX 1
#define TARGET_CMDLINE_START_ARGV_INDEX 2

int main(int argc, char **argv)
{
    const char *progname = argv[0];

    if (argc <= PID_ARGV_INDEX) {
        fprintf(stderr, "Not enough arguments\n");
        help(stderr, progname);
        exit(EXIT_FAILURE);
    }

    pid_t target_pid = parse_pid(argv[PID_ARGV_INDEX], progname);

    struct clone_args cl_args = {
        .flags = CLONE_VFORK | /* Wait for the cloned process to exec() before returning; helps to keep our output separate from the target's. */
                 CLONE_PARENT, /* Make the target process a sibling of this process rather than a child. */
        .pidfd = 0,
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = 0,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = (uintptr_t)&target_pid,
        .set_tid_size = 1,
        .cgroup = 0
    };

    long ret = syscall(SYS_clone3, &cl_args, sizeof(cl_args));

    if (ret == -1) {
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "clone() failed");
    }
    else if (ret == 0) {
        pid_t current_pid = getpid();
        if (target_pid >= 0 && current_pid != target_pid)
            error_at_line(EXIT_FAILURE, 0, __FILE__, __LINE__, "Target process has PID %d, expected PID %d", current_pid, target_pid);

        char **target_argv = (char **)malloc(sizeof(char*) * argc - TARGET_CMDLINE_START_ARGV_INDEX + 1);
        for (int i = TARGET_CMDLINE_START_ARGV_INDEX; i < argc; i++)
            target_argv[i - TARGET_CMDLINE_START_ARGV_INDEX] = argv[i];
        target_argv[argc - TARGET_CMDLINE_START_ARGV_INDEX] = NULL;

        if (execvp(target_argv[0], &target_argv[0]) == -1) {
            free(target_argv);
            error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "exec(%s) failed", argv[TARGET_CMDLINE_START_ARGV_INDEX]);
        }
    }

    return 0;
}
