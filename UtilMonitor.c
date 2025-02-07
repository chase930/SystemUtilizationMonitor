#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

typedef struct {
    int samples;
    int tdelay;         
    int show_memory;   
    int show_cpu;       
    int show_cores;   
} Params;

typedef struct {
    int num_cores;
    double max_freq_mhz;
} CoresInfo;


Params parseArguments(int argc, char **argv);
int getCPUSample(unsigned long long *idle, unsigned long long *total);
double getMemoryUsage(void);
double getTotalMemory(void);
double getSelfMemoryUsage(void);
CoresInfo getCoresInfo(void);
void plotMemoryGraph(double *values, int count, double total_mem);
void plotCPUGraph(double *values, int count);
void displayCoresDiagram(CoresInfo info);


int main(int argc, char **argv) {
    int i;
    Params params = parseArguments(argc, argv);

    double *mem_values = NULL;
    double *cpu_values = NULL;
    if (params.show_memory) {
        mem_values = malloc(params.samples * sizeof(double));
        if (!mem_values) {
            perror("malloc mem_values");
            return 1;
        }
    }
    if (params.show_cpu) {
        cpu_values = malloc(params.samples * sizeof(double));
        if (!cpu_values) {
            perror("malloc cpu_values");
            return 1;
        }
    }

    double total_mem = 0;
    if (params.show_memory)
        total_mem = getTotalMemory();

    CoresInfo cores_info;
    if (params.show_cores)
        cores_info = getCoresInfo();

    unsigned long long prev_idle = 0, prev_total = 0, curr_idle = 0, curr_total = 0;
    if (params.show_cpu) {
        if (getCPUSample(&prev_idle, &prev_total) != 0) {
            fprintf(stderr, "Error reading CPU sample\n");
            return 1;
        }
    }
    double self_mem_usage = 0.0; 

    for (i = 0; i < params.samples; i++) {
        usleep(params.tdelay);

        if (params.show_cpu) {
            if (getCPUSample(&curr_idle, &curr_total) != 0) {
                fprintf(stderr, "Error reading CPU sample\n");
                break;
            }
            unsigned long long delta_idle = curr_idle - prev_idle;
            unsigned long long delta_total = curr_total - prev_total;
            double cpu_usage = 0.0;
            if (delta_total != 0)
                cpu_usage = 100.0 * (delta_total - delta_idle) / (double)delta_total;
            cpu_values[i] = cpu_usage;
            prev_idle = curr_idle;
            prev_total = curr_total;
        }

        if (params.show_memory) {
            double mem_usage = getMemoryUsage();
            mem_values[i] = mem_usage;
            self_mem_usage = getSelfMemoryUsage();
        }

        printf("\033[2J\033[H");
        printf("Running parameters:\n");
        printf("  Samples: %d\n", params.samples);
        printf("  Delay: %d microseconds\n", params.tdelay);
        if (params.show_memory)
            printf("  Monitoring Tool Memory Usage: %.2f MB\n", self_mem_usage);
        printf("\n");

        if (params.show_memory) {
            printf("Memory Usage: Current used: %.2f GB (Total: %.2f GB)\n", mem_values[i], total_mem);
            plotMemoryGraph(mem_values, i + 1, total_mem);
            printf("\n");
        }
        if (params.show_cpu) {
            printf("CPU Usage: Current: %.2f%%\n", cpu_values[i]);
            plotCPUGraph(cpu_values, i + 1);
            printf("\n");
        }
        if (params.show_cores) {
            displayCoresDiagram(cores_info);
            printf("\n");
        }
        fflush(stdout);
    }

    if (params.show_memory) {
        double sum = 0.0;
        for (i = 0; i < params.samples; i++)
            sum += mem_values[i];
        printf("Average Memory Usage: %.2f GB\n", sum / params.samples);
    }
    if (params.show_cpu) {
        double sum = 0.0;
        for (i = 0; i < params.samples; i++)
            sum += cpu_values[i];
        printf("Average CPU Usage: %.2f%%\n", sum / params.samples);
    }

    /* Clean up */
    if (mem_values) free(mem_values);
    if (cpu_values) free(cpu_values);

    return 0;
}


Params parseArguments(int argc, char **argv) {
    Params params;
    params.samples = 20;
    params.tdelay = 500000; 
    /* Initialize flags */
    int flag_memory = 0, flag_cpu = 0, flag_cores = 0;
    int flagsFound = 0;
    int posArgIndex = 1;

    if (posArgIndex < argc && argv[posArgIndex][0] != '-') {
        params.samples = atoi(argv[posArgIndex]);
        posArgIndex++;
    }
    if (posArgIndex < argc && argv[posArgIndex][0] != '-') {
        params.tdelay = atoi(argv[posArgIndex]);
        posArgIndex++;
    }
    for (int i = posArgIndex; i < argc; i++) {
        if (strncmp(argv[i], "--memory", 8) == 0) {
            flag_memory = 1;
            flagsFound = 1;
        } else if (strncmp(argv[i], "--cpu", 5) == 0) {
            flag_cpu = 1;
            flagsFound = 1;
        } else if (strncmp(argv[i], "--cores", 7) == 0) {
            flag_cores = 1;
            flagsFound = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0) {
            params.samples = atoi(argv[i] + 10);
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
            params.tdelay = atoi(argv[i] + 9);
        } else {
        }
    }
    if (flagsFound) {
        params.show_memory = flag_memory;
        params.show_cpu    = flag_cpu;
        params.show_cores  = flag_cores;
    } else {
        params.show_memory = 1;
        params.show_cpu    = 1;
        params.show_cores  = 1;
    }
    return params;
}


int getCPUSample(unsigned long long *idle, unsigned long long *total) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("fopen /proc/stat");
        return -1;
    }
    char line[256];
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    unsigned long long user, nice, system, idle_time, iowait, irq, softirq, steal;
    int num = sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
                     &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);
    if (num < 4)
        return -1;
    *idle = idle_time + iowait;
    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    return 0;
}


double getMemoryUsage(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("fopen /proc/meminfo");
        return 0;
    }
    char line[256];
    double memTotal = 0, memAvailable = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %lf kB", &memTotal) == 1) {
            /* got total */
        }
        if (sscanf(line, "MemAvailable: %lf kB", &memAvailable) == 1) {
            /* got available */
        }
        if (memTotal > 0 && memAvailable > 0)
            break;
    }
    fclose(fp);
    double memUsed = memTotal - memAvailable; // in kB
    return memUsed / (1024 * 1024);           // convert to GB
}

double getTotalMemory(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("fopen /proc/meminfo");
        return 0;
    }
    char line[256];
    double memTotal = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %lf kB", &memTotal) == 1)
            break;
    }
    fclose(fp);
    return memTotal / (1024 * 1024); // convert to GB
}


double getSelfMemoryUsage(void) {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) {
        perror("fopen /proc/self/status");
        return 0;
    }
    char line[256];
    double vmrss = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lf kB", &vmrss);
            break;
        }
    }
    fclose(fp);
    return vmrss / 1024;  
}


CoresInfo getCoresInfo(void) {
    CoresInfo info;
    info.num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    info.max_freq_mhz = 0.0;
    char path[256];
    for (int i = 0; i < info.num_cores; i++) {
        snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i);
        FILE *fp = fopen(path, "r");
        if (fp) {
            long freq_khz = 0;
            if (fscanf(fp, "%ld", &freq_khz) == 1) {
                double freq_mhz = freq_khz / 1000.0;
                if (freq_mhz > info.max_freq_mhz)
                    info.max_freq_mhz = freq_mhz;
            }
            fclose(fp);
        }
    }
    if (info.max_freq_mhz == 0.0) {
        FILE *fp = fopen("/proc/cpuinfo", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "cpu MHz", 7) == 0) {
                    double mhz;
                    if (sscanf(line, "cpu MHz\t: %lf", &mhz) == 1) {
                        if (mhz > info.max_freq_mhz)
                            info.max_freq_mhz = mhz;
                    }
                }
            }
            fclose(fp);
        }
    }
    return info;
}

void plotMemoryGraph(double *values, int count, double total_mem) {
    int height = 10;
    int row, i;
    printf("Memory Utilization Graph (GB):\n");
    for (row = height; row >= 0; row--) {
        double threshold = (total_mem * row) / height;
        if (row == height || row == height/2 || row == 0)
            printf("%5.2f | ", threshold);
        else
            printf("       | ");
        for (i = 0; i < count; i++) {
            if (values[i] >= threshold)
                printf("#");
            else
                printf(" ");
        }
        printf("\n");
    }
    printf("       +");
    for (i = 0; i < count; i++)
        printf("-");
    printf("\n");
    printf("        ");
    for (i = 0; i < count; i++) {
        if (i % 10 == 0)
            printf("%d", i/10);
        else
            printf(" ");
    }
    printf("\n");
}


void plotCPUGraph(double *values, int count) {
    int height = 10;
    int row, i;
    printf("CPU Utilization Graph (%%):\n");
    for (row = height; row >= 0; row--) {
        double threshold = (100.0 * row) / height;
        if (row == height || row == height/2 || row == 0)
            printf("%5.1f%% | ", threshold);
        else
            printf("       | ");
        for (i = 0; i < count; i++) {
            if (values[i] >= threshold)
                printf(":");
            else
                printf(" ");
        }
        printf("\n");
    }
    printf("       +");
    for (i = 0; i < count; i++)
        printf("-");
    printf("\n");
    printf("        ");
    for (i = 0; i < count; i++) {
        if (i % 10 == 0)
            printf("%d", i/10);
        else
            printf(" ");
    }
    printf("\n");
}

void displayCoresDiagram(CoresInfo info) {
    printf("Cores: %d, Max Frequency: %.2f MHz\n", info.num_cores, info.max_freq_mhz);
    printf("Cores Diagram:\n");
    for (int i = 0; i < info.num_cores; i++)
        printf("[CPU%d] ", i);
    printf("\n");
}
