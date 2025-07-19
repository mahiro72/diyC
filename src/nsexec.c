/* ns_exec.c

   Create a child process that executes a shell command in new
   namespace(s);
   Copyright (C) 2017  Vilibald Wanča

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <getopt.h>

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

# define die(msg)                                     \
do {                                                  \
    perror(msg);                                      \
    exit(EXIT_FAILURE);                               \
} while (0)

#define LOG(x...) { if (verbose) { fprintf(stdout, x); fprintf(stdout, "\n"); } }

// 子プロセス(コンテナ)に渡す引数
struct child_args {
    char *hostname;
    char **argv;
    int pipe_fd[2]; // 親プロセスと子プロセスの同期用。[0]: 読み取りFD、[1]: 書き込みFD。親から子への開始通知の際に利用。
};

// 子プロセスのスタックを確保するための構造体
struct clone_stack {
    char stack[4096] __attribute__ ((aligned(16)));
    char ptr[0]; //スタックの終端を指すポインタ
};

// ログ出力の有効/無効を管理するフラグ
static int verbose;

static void usage(char *name) {
    printf("Create a child process that executes a shell command in new namespace(s),\n");
    printf("Usage: %s [OPTIONS] <CMD>\n\n", name);

    printf("\
    -h, --help           print this help\n");
    printf("\
    -n, --net            new network namespace\n");
    printf("\
    -p, --pid            new PID namespace\n");
    printf("\
    -u, --uts HOSTNAME   new UTS namespace\n");
    printf("\
    -v, --verbose        more verbose output\n\n");
    printf("\
    <CMD>                command to be executed\n");

    exit(EXIT_FAILURE);
}

// cloneした子プロセスで実行される関数
// 新しい名前空間での初期化処理や、引数で渡されたコマンドを実行
static int childFunc(void *arg) {
    struct child_args *args = (struct child_args *) arg;
    char ch;
    int err = 0;

    // 書き込みを閉じる(書き込み権限の放棄)
    close(args->pipe_fd[1]);

    // 親からの開始信号を待つ
    if (read(args->pipe_fd[0], &ch, 1) != 0) { //親からのEOFを待つ
        die("Failure in child: read from pipe returned != 0\n");
    }

    // マウント名前空間の初期化 (このプロセス内でのマウント操作を他のプロセスに伝播しない)
    if (mount("/", "/", "none", MS_PRIVATE | MS_REC, NULL) < 0 ) {
        die("mount private");
    }

    // 古いプロセス情報の切り離し (現在の/procにはホストシステムのすべてのプロセス情報がマウントされているので、それを切り離して見えないようにする)
    if (umount2("/proc", MNT_DETACH) < 0) die("unmount proc");
    if (mount("proc", "/proc", "proc", 0, NULL) < 0) die("mount proc");
    LOG("/proc mounted");

    if (args->hostname) {
        sethostname(args->hostname, strlen(args->hostname));
    }

    // PID namespace内でコマンドを実行
    LOG("Executing %s\n", args->argv[0]);
    err = execvp(args->argv[0], args->argv); // 例: execvp("bash", ["bash", NULL])
    if (0 != err) {
        printf("execvp error: %s\n", strerror(errno));
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int flags = SIGCHLD | CLONE_NEWNS;
    int long_index = 0;
    int opt;
    pid_t child_pid;
    struct child_args args;
    struct clone_stack stack;

    verbose = 0;
    args.hostname = NULL;

    static const struct option long_opts[] = {
        { "help", no_argument, NULL, 'h' },
        { "net", no_argument, NULL, 'n' },
        { "pid", no_argument, NULL, 'p' },
        { "uts", required_argument, NULL, 'u' },
        { "verbose", no_argument, NULL, 'v' },
        { NULL, 0, NULL, 0 }
    };

    while ((opt = getopt_long(argc, argv, "+inpu:v", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 'n': flags |= CLONE_NEWNET; break;
            case 'p': flags |= CLONE_NEWPID; break;
            case 'u':
                flags |= CLONE_NEWUTS;
                args.hostname = optarg;
                break;
            case 'v': verbose = TRUE; break;
            default: usage(argv[0]);
        }
    }

    args.argv = &argv[optind];

    // パイプ作成 (親/子の同期用)
    if (pipe(args.pipe_fd) == -1) die("pipe");

    // 子プロセスを作成
    child_pid = clone(childFunc, stack.ptr, flags, &args);
    if (child_pid == -1) die("clone");

    LOG("%s: child created with PID %ld\n", argv[0], (long) child_pid);

    // 子プロセスに実行開始をEOFで通知
    close(args.pipe_fd[1]);

    LOG("waiting for child to terminate\n");
    if (waitpid(child_pid, NULL, 0) == -1) die("waitpid");

    LOG("%s: terminating\n", argv[0]);

    exit(EXIT_SUCCESS);
}
