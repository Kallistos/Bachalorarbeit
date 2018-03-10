#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

unsigned int (*latency) (unsigned int);
int nmbr_instructions = 1;

void benchmark (const unsigned int N, float freq, char * instr_name) {
    struct timeval start, end;
    double benchtime;
    unsigned int result;


    // run benchmark for instruction instr_name:
	
    gettimeofday (&start, NULL);
    result = (*latency)(N);
    gettimeofday (&end, NULL);

    benchtime = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    benchtime = benchtime / (1e6 * nmbr_instructions / freq * (N / 1e9));

    fprintf (stdout, "%s:%s\t%.3f (clock cycles)\t[Debug - result:%llu]\n", instr_name, strlen(instr_name) + 1 < 8 ? "\t" : "", benchtime, result);
}

int main (int argc, const char** argv){
    const unsigned int N = 10000;
    float freq = 0.0f;

    if (argc < 2) {
        fprintf (stdout, "please specify a directory containing the x86 functions with benchmarks to run!\n");
        exit(EXIT_FAILURE);
    }
    if (argc < 3) {
        fprintf (stdout, "please specify a frequency to run the benchmarks with (in GHz). Make sure the frequency is fixed for best results.\n");
        exit(EXIT_FAILURE);
    }
    freq = atof(argv[2]);
    fprintf (stdout, "Using frequency %.2fGHz.\n", freq);

    DIR* p_dir;
    struct dirent * p_d;

    if ((p_dir = opendir(argv[1])) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while ((p_d = readdir(p_dir)) != NULL) {
        //only try .s files
        char *suffix = ".s";
        int len_suffix = strlen(suffix);
        if (strncmp(p_d->d_name + strlen(p_d->d_name) - len_suffix, suffix, len_suffix))
            continue;
        //load .s
        void* handle;
        size_t len_1 = strlen(argv[1]);
        size_t len_2 = strlen(p_d->d_name);
        // directory might be missing a trailing '/'
        char* rel_path;
        if ((rel_path = malloc(len_1 + len_2 + 2)) == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        snprintf(rel_path, len_1 + len_2 + 2, "%s/%s", argv[1], p_d->d_name);
        if ((handle = dlopen(rel_path,RTLD_LAZY)) == NULL) {
            fprintf(stderr, "dlopen: failed to open %s: %s.\n", rel_path, dlerror());
            exit(EXIT_FAILURE);
        }
        // add some feature to read latency and numbr instructions from file! 
        if ((latency = (unsigned int (*) (unsigned int))dlsym(handle, "latency")) == NULL) {
            perror ("could not find function in file");
            exit (EXIT_FAILURE);
        }
        // doc actual benchmark
        char* instr = strtok(p_d->d_name, ".");
        benchmark(N, freq, instr);
        dlclose(handle);
    }
    return 0;
}
