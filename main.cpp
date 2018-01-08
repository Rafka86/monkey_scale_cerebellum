#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pzcl/pzcl.h>
#include <mpi.h>

#define MAX_DEVICE_IDS (8)

//#include "param.h"
#define MAX_BINARY_SIZE (0x1000000)
#define KERNEL_FILE "./kernel.sc2/kernel.sc2.pz"
#define KERNEL_FUNC "kernel"

static struct timeval start, stop;
void timer_start(void) {
  gettimeofday(&start, NULL);
}
double timer_elapsed(void) {
  struct timeval elapsed;
  gettimeofday(&stop, NULL);
  timersub(&stop, &start, &elapsed);
  return (elapsed.tv_sec * 1000.0 + elapsed.tv_usec / 1000.0);
}

int main (int argc, char *argv[]) {
  // MPI Setup
  MPI_Init(NULL, NULL);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  fprintf(stderr, "Hello world from processor %s, rank %d out of %d processors\n",
          processor_name, world_rank, world_size);

  // PEZY Setup
  /*
  pzcl_platform_id platform;
  int stats = pzclGetPlatformIDs (1, &platform, NULL);
  if (stats != 0) {fprintf(stderr, "pzclGetPlatformIDs: %d\n", stats); exit(1);}
 
  pzcl_device_id device[MAX_DEVICE_IDS];
  pzcl_uint num_devices;
  stats = pzclGetDeviceIDs(platform, PZCL_DEVICE_TYPE_DEFAULT, MAX_DEVICE_IDS, device, &num_devices);
  if (stats != 0) {fprintf(stderr, "pzclGetDeviceIDs: %d\n", stats); exit(1);}

  for (int i = 0; i < num_devices; i++) {
    char name_data [128];
    pzcl_int err = pzclGetDeviceInfo(device[i], PZCL_DEVICE_NAME, sizeof(name_data), name_data, NULL);
    printf("%s%s", name_data, (i == num_devices - 1) ? "\n" : " ");
  }
  */

  // Finalize MPI environment
  MPI_Finalize();

  return 0;
}
