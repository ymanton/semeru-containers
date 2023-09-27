#include <linux/sched.h>    /* Definition of struct clone_args */
#include <sched.h>          /* Definition of CLONE_* constants */
#include <sys/syscall.h>    /* Definition of SYS_* constants */
#include <signal.h>         /* Definition of SIGCHLD */
#include <unistd.h>
#include <error.h>          /* Definition of error_at_line */
#include <stdlib.h>         /* Definition of EXIT_FAILURE */
#include <errno.h>
#include <stdio.h>          /* Definition of printf */

extern char *optarg;
extern int optind, opterr, optopt;

pid_t parse_options(int argc, char **argv, int &target_cmdline_start) {
    pid_t target_pid = -1;
    int opt = -1;
    /* Note that our getopt() optstring starts with + because we need POSIXLY_CORRECT behaviour. The target may have its own
       command-line options and we don't want to process those nor do we want getopt() to transmute argv. If we don't enable
       POSIXLY_CORRECT mode we would need "--" between this program and its options and the target and it's command-line. */
    while ((opt = getopt(argc, argv, "+p:h")) != -1) {
        switch (opt) {
            case 'p':
                target_pid = atoi(optarg);
                break;
            case 'h':
                printf("TODO: help text\n");
                break;
            case '?':
                fprintf(stderr, "Invalid option or option argument\n");
                printf("TODO: help text\n");
                exit(EXIT_FAILURE);
                break;
            default:
                fprintf(stderr, "getopt() failed and returned %d\n", opt);
                printf("TODO: help text\n");
                exit(EXIT_FAILURE);
                break;
        }
    }
    target_cmdline_start = optind;
    return target_pid;
}

int main(int argc, char **argv)
{
    int target_cmdline_start;
    pid_t target_pid = parse_options(argc, argv, target_cmdline_start);
    struct clone_args cl_args = {
        .flags = CLONE_VFORK | /* Wait for the cloned process to exec() before returning; keeps output tidy. */
                 CLONE_PARENT, /* Make the target process a sibling of this process rather than a child. */
        .pidfd = 0,
        .child_tid = 0,
        .parent_tid = 0,
        .exit_signal = 0,
        .stack = 0,
        .stack_size = 0,
        .tls = 0,
        .set_tid = target_pid < 0 ? (unsigned long long)0 : (unsigned long long)&target_pid,
        .set_tid_size = target_pid < 0 ? (unsigned long long)0 : (unsigned long long)1,
        .cgroup = 0
    };
    long ret = syscall(SYS_clone3, &cl_args, sizeof(cl_args));
    if (ret == -1) {
        error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "clone() failed");
    }
    else if (ret == 0) {
        printf("Success, cloned a process, pid: %d\n", getpid());
        char **target_argv = (char **)malloc(sizeof(char*) * argc - target_cmdline_start + 1);
        for (int i = target_cmdline_start; i < argc; i++)
            target_argv[i - target_cmdline_start] = argv[i];
        target_argv[argc - target_cmdline_start] = NULL;
        {
            int i = 0;
            while (target_argv[i] != NULL) {
                printf("target_argv[%d]=%s\n", i, target_argv[i]);
                i++;
            }
        }
        if (execvp(target_argv[0], &target_argv[0]) == -1)
            error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, "exec(%s) failed", target_argv[0]);
        free(target_argv);
    }
    return 0;
}
