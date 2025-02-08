# SystemUtilizationMonitor

Overview
------------------------
A C program that reports different metrics of the utilization of a given system (e.g. cores, CPU, memory usage). It is designed to target Linux type OS systems. It reads system data from the /proc filesystem, performs appropriate calculations and outputs an updating ASCII graph which will run finitely until it has collected a user-specified number of samples (20 with a 0.5 second delay by default if unspecified).

How I Solved the Problem
------------------------
**Data Collection:**  
Setting up flag readers and graph plotting functions wasn't too difficult and was within the scope of what I had already known. Data collection was the novel task and tackling it with material from CSCB09 and research was very effective in aiding me.
   - *Memory Usage:* `getMemoryUsage` and `getTotalMemory` parse `/proc/meminfo` to calculate current memory usage (in GB) and total available memory.  
   - *Self Memory Usage:* `getSelfMemoryUsage` reads `/proc/self/status` to determine how much memory the monitoring tool itself is consuming (in MB).  
   - *CPU Usage:* The `getCPUSample` function reads `/proc/stat` to determine CPU time fields. The tool calculates CPU utilization in percent by computing differences between consecutive samples. 
   - *Core Information:* `getCoresInfo` uses `sysconf` to determine the number of available cores and reads each core’s maximum frequency from the sysfs. If sysfs fails, it parses `/proc/cpuinfo`.

Function Overview
-----------------
- **Params parseArguments(int argc, char **argv)**  
  Parses command-line arguments. Allows both positional arguments (samples and tdelay) and flags (--memory, --cpu, --cores, --samples=N, --tdelay=T).  
  *Returns:* A Params structure containing the sample count, delay, and flags indicating which metrics to display.

- **int getCPUSample(unsigned long long *idle, unsigned long long *total)**  
  Reads the first line of `/proc/stat` to get the CPU time fields and calculates the total and idle times.  
  *Returns:* 0 on success, -1 on failure.

- **double getMemoryUsage(void)**  
  Reads `/proc/meminfo` to calculate the used memory (in GB) by subtracting available memory from total memory.  
  *Returns:* Memory usage in GB.

- **double getTotalMemory(void)**  
  Reads `/proc/meminfo` to obtain the total memory (in GB).  
  *Returns:* Total memory in GB.

- **double getSelfMemoryUsage(void)**  
  Reads `/proc/self/status` to determine the self memory usage (in MB) of the monitoring tool.  
  *Returns:* Self memory usage in MB.

- **CoresInfo getCoresInfo(void)**  
  Uses `sysconf` to determine the number of CPU cores and reads sysfs (or falls back to `/proc/cpuinfo`) to obtain the maximum frequency (in MHz).  
  *Returns:* A CoresInfo structure with the number of cores and the max frequency.

- **void plotMemoryGraph(double *values, int count, double total_mem)**  
  Draws an ASCII graph for memory utilization using '#' chars. The y-axis is scaled from 0 to total memory (GB). Each column represents a sample.  
  *Parameters:* Array of memory usage values, the number of samples to display, and total memory.

- **void plotCPUGraph(double *values, int count)**  
  Draws an ASCII graph for CPU utilization using ':' chars. The y-axis represents percentage values from 0% to 100%. Each column represents a sample.  
  *Parameters:* Array of CPU usage values and the number of samples to display.

- **void displayCoresDiagram(CoresInfo info)**  
  Prints the number of cores, maximum frequency, and a diagram showing each core.  
  *Parameters:* CoresInfo structure with core information.


How to Run (Use) the Program
----------------------
1. **Compile the Program:**  
   Ensure SystemUtilizationMonitor is being run on a Linux system and compile the program (e.g. with gcc).
2. **Run**
   The program accepts several flags, including:
   --memory
        to indicate that only the memory usage should be generated
   --cpu
        to indicate that only the CPU usage should be generated
   --cores
        to indicate that only the cores information should be generated
   --samples=N
        if used the value N will indicate how many times the statistics are going to be collected and results will be average and   
        reported based on the N number of repetitions. If not value is indicated the default value will be 20.
   --tdelay=T
        to indicate how frequently to sample in micro-seconds (1 microsec = 10 -6 sec.)
        If no value is indicated the default value will be 0.5 sec = 500 milisec = 500000 microsec.
   *These last two arguments can also be specified as positional arguments if no flag is indicated, in the corresponding order: samples     tdelay. In this case, these argument should be the first ones to be passed to the program.*
