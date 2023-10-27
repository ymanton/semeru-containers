#include <linux/sched.h>    /* Definition of struct clone_args */
#include <sched.h>          /* Definition of CLONE_* constants */
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <signal.h>         /* Definition of SIGCHLD */
#include <unistd.h>
#include <error.h>          /* Definition of error_at_line */
#include <stdlib.h>         /* Definition of EXIT_FAILURE */
#include <errno.h>
#include <stdio.h>          /* Definition of printf */
#include <limits.h>         /* Definition of INT_MIN, INT_MAX */
#include <stdbool.h>        /* Definition of bool */
#include <stdint.h>         /* Definition of uintptr_t */

void help(FILE *stream, const char *progname)
{
    fprintf(stream, "%s [OPTIONS...] [--] [COMMAND [ARGS...]]\n"
                    "Options:\n"
                    "\t-p <PID> Target PID\n"
                    "\t-h Help (this text)\n", progname);
}

extern char *optarg;
extern int optind, opterr, optopt;

pid_t parse_options(int argc, char **argv, int *target_cmdline_start) {
    const char *progname = argv[0];
    pid_t target_pid = -1;
    int opt = -1;

    /* Note that our getopt() optstring starts with + because we need POSIXLY_CORRECT behaviour. The target may have its own
       command-line options and we don't want to process those nor do we want getopt() to transmute argv. If we don't enable
       POSIXLY_CORRECT mode we would need "--" between this program and its options and the target and it's command-line. */
    while ((opt = getopt(argc, argv, "+p:h")) != -1) {
        switch (opt) {
            case 'p':
            {
                bool pid_parsed = false;
                char *optarg_end = NULL;
                long target_pid_long = strtol(optarg, &optarg_end, 0);

                if (*optarg_end != '\0')
                    fprintf(stderr, "Invalid argument to -p: %s\n", optarg);
                else if (errno == ERANGE)
                    fprintf(stderr, "Argument to -p out of range for long: %s\n", optarg);
                else if (target_pid_long < INT_MIN || target_pid_long > INT_MAX)
                    fprintf(stderr, "Argument to -p out of range for pid_t: %s\n", optarg);
                else
                    pid_parsed = true;

                if (!pid_parsed) {
                    help(stderr, progname);
                    exit(EXIT_FAILURE);
                }

                target_pid = target_pid_long;

                break;
            }
            case 'h':
                help(stdout, progname);
                break;
            case '?':
                fprintf(stderr, "Invalid option or option argument\n");
                help(stderr, progname);
                exit(EXIT_FAILURE);
                break;
            default:
                fprintf(stderr, "getopt() failed and returned %d\n", opt);
                help(stderr, progname);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (target_cmdline_start)
        *target_cmdline_start = optind;

    return target_pid;
}

int main(int argc, char **argv)
{
    int target_cmdline_start = -1;
    pid_t target_pid = parse_options(argc, argv, &target_cmdline_start);

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
        .set_tid = target_pid < 0 ? 0 : (uintptr_t)&target_pid,
        .set_tid_size = target_pid < 0 ? 0 : 1,
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

        char **target_argv = (char **)malloc(sizeof(char*) * argc - target_cmdline_start + 1);
        for (int i = target_cmdline_start; i < argc; i++)
            target_argv[i - target_cmdline_start] = argv[i];
        target_argv[argc - target_cmdline_start] = NULL;

        if (execvp(target_argv[0], &target_argv[0]) == -1) {
            free(target_argv);
            error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "exec(%s) failed", argv[target_cmdline_start]);
        }
    }

    return 0;
}
