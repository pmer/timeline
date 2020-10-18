#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__

#include <mach/mach_time.h>

static struct mach_timebase_info timebase = {0, 0};

uint64_t ns() {
    if (timebase.numer == 0 && timebase.denom == 0) {
        kern_return_t err = mach_timebase_info(&timebase);
        assert(err == KERN_SUCCESS);
    }
    return mach_absolute_time() * timebase.numer / timebase.denom;
}

#else

uint64_t ns() {
    struct timespec tp;
    int err = clock_gettime(CLOCK_UPTIME_RAW, &tp);
    assert(err == 0);
    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}

#endif

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

        start = (double)ns() / 1000000000;
        prev = start;

        while ((linelen = getline(&line, &linecap, masterfile)) > 0) {
            now = (double)ns() / 1000000000;

            bytes = printf("%c[0m[%.3f/%.3f] ", 27, now - prev, now - start);
            assert(bytes >= 5);

            prev = now;

            bytes = fwrite(line, linelen, 1, stdout);
            assert(bytes == 1);
        }
    }
    return 0;
}
