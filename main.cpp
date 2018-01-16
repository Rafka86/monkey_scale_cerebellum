#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <pzcl/pzcl.h>
#include <mpi.h>

#define MAX_DEVICE_IDS (8)

#include "param.h"
#define MAX_BINARY_SIZE (0x1000000)
#define KERNEL_FILE "./kernel.sc2/kernel.sc2.pz"
#define KERNEL_FUNC "kernel"

void Error(const char* message, ...) {
  va_list ap;
  va_start(ap, message);
  fprintf(stderr, "Error : ");
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

int compare_unsigned_short(const void* a, const void* b) {
  return *(unsigned short*)a - *(unsigned short*)b;
}
void setup_grlist(unsigned short* list_gogr, unsigned short* list_grgo) {
  int gox, goy;
  int goax, goay, godx, gody;
  int gon, goan;
  int nlist_gogr[N_GO], nlist_grgo[N_GO];

  for (int i = 0; i < N_GO; i++) nlist_gogr[i] = 0;
  for (int i = 0; i < N_GO; i++) nlist_grgo[i] = 0;

  for (goy = 0; goy < Y_GO; goy++) {
    for (gox = 0; gox < X_GO; gox++) {
      gon = goy * X_GO + gox;
      for (int i = 0; i < N_GR_PER_GO; i++) {
        godx = -3 + (int)(7.0f * (float)rand() / (float)RAND_MAX);
        gody = -3 + (int)(7.0f * (float)rand() / (float)RAND_MAX);
        goax = gox + godx + 3 * X_GO / 4;
        if (goax >= X_GO) goax -= X_GO;
        if (goax < 0) goax += X_GO;
        goay = goy + gody + Y_GO / 2;
        if (goay >= Y_GO) goay -= Y_GO;
        if (goay < 0) goay += Y_GO;
        goan = goay * X_GO + goax;
        list_gogr[gon * N_GO + i] = goan;
        nlist_gogr[gon]++;
      }
    }
  }

  int** arr;
  int naxons[N_GO];
  arr = (int**)malloc(sizeof(int*) * N_GO);
  for (int i = 0; i < N_GO; i++) arr[i] = (int*)malloc(sizeof(int) * N_GO);
  for (int i = 0; i < N_GO; i++) naxons[i] = 0;

  for (goy = 0; goy < Y_GO; goy++) {
    for (gox = 0; gox < X_GO; gox++) {
      gon = goy * X_GO + gox;
      for (int i = 0; i < N_GO_PER_GR / 4; i++) {
        godx = -4 + (int)(9.0f * (float)rand() / (float)RAND_MAX);
        gody = -4 + (int)(9.0f * (float)rand() / (float)RAND_MAX);
        goax = gox + godx;
        if (goax >= X_GO) goax -= X_GO;
        if (goax < 0) goax += X_GO;
        goay = goy + gody;
        if (goay >= Y_GO) goay -= Y_GO;
        if (goay < 0) goay += Y_GO;
        goan = goay * X_GO + goax;
        arr[gon][naxons[gon]] = goan;
        naxons[gon]++;
      }
    }
  }
  int grn;
  for (gox = 0; gox < X_GO; gox++) {
    for (goy = 0; goy < Y_GO; goy++) {
      gon = goy * X_GO + gox;
      grn = gon;
      for (godx = 0; godx <= 1; godx++) {
        goax = gox + godx;
        if (goax >= X_GO) goax -= X_GO;
        for (gody = 0; gody <= 1; gody++) {
          goay = goy + gody;
          if (goay >= Y_GO) goay -= Y_GO;
          goan = goay * X_GO + goax;
          for (int i = 0; i < naxons[goan]; i++) {
            list_grgo[grn * N_GO + nlist_grgo[grn]] = arr[goan][i];
            nlist_grgo[grn]++;
          }
        }
      }
    }
  }
  for (int i = 0; i < N_GO; i++) free(arr[i]);
  free(arr);

  // Check the number of afferent inputs
  for (int i = 0; i < N_GO; i++) {
    if (nlist_grgo[i] != N_GO_PER_GR) Error("nlist_grgo[%d] = %d, N_GO_PER_GR = %d\n", i, nlist_grgo[i], N_GO_PER_GR);
    if (nlist_gogr[i] != N_GR_PER_GO) Error("nlist_gogr[%d] = %d, N_GR_PER_GO = %d\n", i, nlist_gogr[i], N_GR_PER_GO);
  }

  // Sort list to make accessing easy.
  for (int i = 0; i < N_GO; i++) {
    qsort(&list_grgo[N_GO * i], N_GO_PER_GR, sizeof(unsigned short), compare_unsigned_short);
    qsort(&list_gogr[N_GO * i], N_GR_PER_GO, sizeof(unsigned short), compare_unsigned_short);
  }

  // Check the number of lists.
  for (int i = 0; i < N_GO; i++) {
    for (int j = 0; j < N_GO_PER_GR; j++)
      if (list_grgo[i * N_GO + j] < 0 || list_grgo[i * N_GO + j] >= N_GO)
        Error("Failed to make list_grgo\n");
    for (int j = 0; j < N_GR_PER_GO; j++)
      if (list_gogr[i * N_GO + j] < 0 || list_gogr[i * N_GO + j] >= N_GO)
        Error("Failed to make list_gogr\n");
  }
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

  //
  // Data Setup
  //

  unsigned long* s_gr = (unsigned long*)malloc(sizeof(unsigned long) * N_S_GR);
  if (s_gr == NULL) Error("failed malloc s_gr\n");
  for (int i = 0; i < N_S_GR; i++) s_gr[i] = 0;
  pzcl_mem dev_s_gr = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned long) * N_S_GR, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_gr\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_gr, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR, s_gr, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_gr\n", err);

  unsigned char* s_go = (unsigned char*)malloc(sizeof(unsigned char) * N_GO);
  if (s_go == NULL) Error("failed malloc s_go\n");
  for (int i = 0; i < N_GO; i++) s_go[i] = 0;
  pzcl_mem dev_s_go = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned char) * N_GO, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_go\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_go, PZCL_TRUE, 0, sizeof(unsigned char) * N_GO, s_go, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_go\n", err);

  srand(23L);
  unsigned int* seeds = (unsigned int*)malloc(sizeof(unsigned int) * N_GR);
  if (seeds == NULL) Error("failed malloc seeds\n");
  for (int i = 0; i < N_GR; i++) seeds[i] = rand();
  pzcl_mem dev_seeds = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned int) * N_GR, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_seeds\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_seeds, PZCL_TRUE, 0, sizeof(unsigned int) * N_GR, seeds, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d seeds\n", err);

  unsigned short* lists = (unsigned short*)malloc(sizeof(unsigned short) * N_GO * N_GO * 2);
  if (lists == NULL) Error("Failed malloc lists\n");
  unsigned short* list_gogr = lists;
  unsigned short* list_grgo = lists + N_GO * N_GO;
  setup_grlist(list_gogr, list_grgo);
  pzcl_mem dev_lists = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned short) * N_GO * N_GO * 2, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_lists\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_lists, PZCL_TRUE, 0, sizeof(unsigned short) * N_GO * N_GO * 2, lists, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d lists\n", err);

  float* u = (float*)malloc(sizeof(float) * N_ALL);
  if (u == NULL) Error("failed malloc u\n");
  for (int i = 0; i < N_ALL; i++) {
    float e_leak = ( GR(i) * (E_LEAK_GR) 
                    +GO(i) * (E_LEAK_GO));
    u[i] = e_leak;
  }
  pzcl_mem dev_u = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_u\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_u, PZCL_TRUE, 0, sizeof(float) * N_ALL, u, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d u\n", err);

  float* g_ex = (float*)malloc(sizeof(float) * N_ALL);
  if (g_ex == NULL) Error("failed malloc g_ahp\n");
  for (int i = 0; i < N_ALL; i++) g_ex[i] = 0.0f;
  pzcl_mem dev_g_ex = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_g_ex\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_g_ex, PZCL_TRUE, 0, sizeof(float) * N_ALL, g_ex, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d g_ex\n", err);

  float* g_ahp = (float*)malloc(sizeof(float) * N_ALL);
  if (g_ahp == NULL) Error("failed malloc g_ahp\n");
  for (int i = 0; i < N_ALL; i++) g_ahp[i] = 0.0f;
  pzcl_mem dev_g_ahp = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_g_ahp\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_g_ahp, PZCL_TRUE, 0, sizeof(float) * N_ALL, g_ahp, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d g_ahp\n", err);

  char* spikep_buf = (char*)malloc(sizeof(char) * N_ALL_P * T_I);
  if (spikep_buf == NULL) Error("failed malloc spikep_buf\n");
  for (int i = 0; i < N_ALL_P * T_I; i++) spikep_buf[i] = 0;
  pzcl_mem dev_spikep_buf = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(char) * N_ALL_P * T_I, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_spikep_buf\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_spikep_buf, PZCL_TRUE, 0, sizeof(char) * N_ALL_P * T_I, spikep_buf, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d spikep_buf\n", err);

  char* h_spikep_buf = (char*)malloc(sizeof(char) * N_ALL_P * N_PERIOD);

  size_t work_unit_size = WORK_UNIT_SIZE;
  pzcl_event event = NULL;

  for (int nt = 0; nt < N_TRIALS; nt++) {
    FILE* fgr_spk;
    FILE* log;
    if (world_rank == 0) {
      char fn[1024];
      sprintf(fn, "gr.spk.%d.%d", world_rank, nt);
      fgr_spk = fopen(fn, "w");
      sprintf(fn, "log.%d.%d", world_rank, nt);
      log = fopen(fn, "w");
      fprintf(stderr, "== %2d ==\n", nt);
    }

    pzclSetKernelArg(kernel, 0, sizeof(pzcl_mem), &dev_seeds);
    pzclSetKernelArg(kernel, 1, sizeof(pzcl_mem), &dev_lists);
    pzclSetKernelArg(kernel, 2, sizeof(pzcl_mem), &dev_s_gr);
    pzclSetKernelArg(kernel, 3, sizeof(pzcl_mem), &dev_s_go);
    pzclSetKernelArg(kernel, 4, sizeof(pzcl_mem), &dev_u);
    pzclSetKernelArg(kernel, 5, sizeof(pzcl_mem), &dev_g_ex);
    pzclSetKernelArg(kernel, 6, sizeof(pzcl_mem), &dev_g_ahp);
    pzclSetKernelArg(kernel, 7, sizeof(pzcl_mem), &dev_spikep_buf);

    if (world_rank == 0) timer_start();

    int count = 0;
    for (int t_e = 0; t_e < N_PERIOD; t_e += T_I) {
      count++;

      pzclSetKernelArg(kernel, 8, sizeof(int), &t_e);

      err = pzclEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_unit_size, NULL, 0, NULL, &event);
      if (err) Error("pzclEnqueueNDRangeKernel: %d\n", err);
      if (event) {
        err = pzclWaitForEvents(1, &event);
        if (err) Error("pzclWaitForEvents: %d in trial.\n", err);
      }

      err = pzclEnqueueReadBuffer(queue, dev_spikep_buf, PZCL_TRUE, 0, sizeof(char) * N_ALL_P * T_I, spikep_buf, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      if (spikep_buf == NULL) Error("Enpty spikep_buf.\n");
#ifdef DEBUG
      err = pzclEnqueueReadBuffer(queue, dev_u, PZCL_TRUE, 0, sizeof(float) * N_ALL, u, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      fprintf(log, "v:%f\n", u[N_GR]);
      err = pzclEnqueueReadBuffer(queue, dev_g_ex, PZCL_TRUE, 0, sizeof(float) * N_ALL, g_ex, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      fprintf(log, "g_ex:%f\n", g_ex[N_GR]);
#endif

      for (int t_i = 0; t_i < T_I; t_i++) {
        int t_each = t_e + t_i;
        for (int i = 0; i < N_ALL_P; i++) {
          h_spikep_buf[t_each * N_ALL_P + i] = spikep_buf[t_i * N_ALL_P + i];
        }
      }
    }

    if (world_rank == 0) {
      double elapsed_time = timer_elapsed();
      fprintf(stderr, "time: %f msec\n", elapsed_time);

      for (int t = 0; t < N_PERIOD; t++) {
        for (int i = 0; i < N_ALL_P; i++)
          if (h_spikep_buf[t * N_ALL_P + i] != 0) fprintf(fgr_spk, "%d %d\n", t, i);
      }
      fclose(fgr_spk);
      fclose(log);
    }
  }

  // Release memory objects
  free(seeds);
  free(lists);
  free(s_gr);
  free(s_go);
  free(u);
  free(g_ex);
  free(g_ahp);
  free(spikep_buf);

  // Release PZCL Resources
  pzclReleaseEvent(event);
  pzclReleaseMemObject(dev_seeds);
  pzclReleaseMemObject(dev_lists);
  pzclReleaseMemObject(dev_s_gr);
  pzclReleaseMemObject(dev_s_go);
  pzclReleaseMemObject(dev_u);
  pzclReleaseMemObject(dev_g_ex);
  pzclReleaseMemObject(dev_g_ahp);
  pzclReleaseMemObject(dev_spikep_buf);
  pzclReleaseCommandQueue(queue);
  pzclReleaseKernel(kernel);
  pzclReleaseProgram(program);
  pzclReleaseContext(context);

  // Finalize MPI environment
  MPI_Finalize();

  return 0;
}
