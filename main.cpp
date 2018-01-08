#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <pzcl/pzcl.h>
#include <mpi.h>

#define MAX_DEVICE_IDS (8)

//#include "param.h"
#define MAX_BINARY_SIZE (0x1000000)
#define KERNEL_FILE "./kernel.sc2/kernel.sc2.pz"
#define KERNEL_FUNC "kernel"

void Error(const char* message, ...) {
  va_list ap;
  va_start(ap, message);
  fprintf(stderr, "Error:");
  vfprintf(stderr, message, ap);
  va_end(ap);
  exit(1);
}

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
  pzcl_platform_id platform;
  int status = pzclGetPlatformIDs (1, &platform, NULL);
  if (status != 0) Error("pzclGetPlatformIDs: %d\n", status);
 
  pzcl_device_id device[MAX_DEVICE_IDS];
  pzcl_uint num_devices;
  status = pzclGetDeviceIDs(platform, PZCL_DEVICE_TYPE_DEFAULT, MAX_DEVICE_IDS,
                            device, &num_devices);
  if (status != 0) Error("pzclGetDeviceIDs: %d\n", status);

  for (int i = 0; i < num_devices; i++) {
    char name_data [128];
    pzcl_int err = pzclGetDeviceInfo(device[i], PZCL_DEVICE_NAME, sizeof(name_data),
                                     name_data, NULL);
    printf("%s%s", name_data, (i == num_devices - 1) ? "\n" : " ");
  }

  int device_id = world_rank % MAX_DEVICE_IDS;
  pzcl_int err;
  pzcl_context context = pzclCreateContext(NULL, 1, &device[device_id], NULL, NULL, &err);
  if (err) Error("pzclCreateContext: %d on %s:%d\n", processor_name, world_rank, err);

  FILE* program_handle = fopen(KERNEL_FILE, "rb");
  char* program_buffer = (char*)malloc(MAX_BINARY_SIZE);
  size_t program_size = fread(program_buffer, 1, MAX_BINARY_SIZE, program_handle);
  fclose(program_handle);
  pzcl_int program_status;
  pzcl_program program = pzclCreateProgramWithBinary(context, 1, &device[device_id], (const size_t*)&program_size,
                                                     (const unsigned char**)&program_buffer, &program_status, &err);
  free(program_buffer);
  if (err) Error("pzclCreateProgramWithBinary: %d status:%d size:%d\n", err, program_status, program_size);

  pzcl_kernel kernel = pzclCreateKernel(program, KERNEL_FUNC, &err);
  if (err) Error("pzclCreateKernel of %s: %d\n", KERNEL_FUNC, err);

  pzcl_command_queue queue = pzclCreateCommandQueue(context, device[device_id], 0, &err);
  if (err) Error("pzclCreateCommandQueue: %d\n", err);

  // Release PZCL Resources
  pzclReleaseCommandQueue(queue);
  pzclReleaseKernel(kernel);
  pzclReleaseProgram(program);
  pzclReleaseContext(context);

  // Finalize MPI environment
  MPI_Finalize();

  return 0;
}
