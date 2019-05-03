#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv) {
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    struct timespec tp;
    int err;
    double start;
    double prev, now;
    int master;
    FILE* masterfile;
    pid_t pid;
    ssize_t bytes;

    master = posix_openpt(O_RDWR);
    assert(master != -1);

    pid = fork();
    assert(pid != -1);

    if (pid == 0) {
        err = grantpt(master);
        assert(err != -1);

        err = unlockpt(master);
        assert(err != -1);

        char* slavepath = ptsname(master);
        assert(slavepath);

        close(master);

        int slave = open(slavepath, O_RDWR);
        assert(slave != -1);

        dup2(slave, 1);
        dup2(slave, 2);

        execvp(argv[1], argv + 1);
        perror("execvp");
    }

    else {
        masterfile = fdopen(master, "r");
        assert(masterfile);

        err = clock_gettime(CLOCK_UPTIME_RAW, &tp);
        assert(err == 0);
        start = (double)tp.tv_sec + (double)tp.tv_nsec / 1000000000;
        prev = start;

        while ((linelen = getline(&line, &linecap, masterfile)) > 0) {
            err = clock_gettime(CLOCK_UPTIME_RAW, &tp);
            assert(err == 0);
            now = (double)tp.tv_sec + (double)tp.tv_nsec / 1000000000;

            bytes = printf("%c[0m[%.3f/%.3f] ", 27, now - prev, now - start);
            assert(bytes >= 5);

            prev = now;

            bytes = fwrite(line, linelen, 1, stdout);
            assert(bytes == 1);
        }
    }
    return 0;
}
