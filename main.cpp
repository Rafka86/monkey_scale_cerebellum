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
#include "../common/pzclutil.h"
#define MAX_BINARY_SIZE (0x1000000)
#define KERNEL_FILE "./kernel.sc2/kernel.sc2.pz"
#define KERNEL_FUNC "kernel"
#define KERNEL_LTD  "ltd"
#define KERNEL_INIT "init_local_memory"
#define KERNEL_FIN  "fin_local_memory"
#define KERNEL_PROF "GetPerformance"

void Error(const char* message, ...) {
  va_list ap;
  va_start(ap, message);
  fprintf(stderr, "Error : ");
  vfprintf(stderr, message, ap);
  va_end(ap);
  exit(1);
}

typedef CL_API_ENTRY cl_int (CL_API_CALL * pfnPezyExtSetPerThreadStackSize)(pzcl_kernel kernel, size_t size);
bool SetStackSize (pzcl_kernel kernel, unsigned int size) {
  bool ret = false;
  pfnPezyExtSetPerThreadStackSize PezyExtSetPerThreadStackSize
    = (pfnPezyExtSetPerThreadStackSize)clGetExtensionFunctionAddress("pezy_set_per_thread_stack_size");
  if (PezyExtSetPerThreadStackSize) {
    ret = PezyExtSetPerThreadStackSize(kernel, size) == CL_SUCCESS;
  }
  return ret;
}

PZCPerformance GetPerformance(pzcl_context context, pzcl_command_queue queue, pzcl_kernel kernel, int index) {
  PZCPerformance perf;
  pzcl_int result = 0;

  pzcl_mem mem = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(PZCPerformance), NULL, &result);
  if (mem == NULL) Error("pzclCreateBuffer failed, %d\n", result);

  pzclSetKernelArg(kernel,  0, sizeof(pzcl_mem), (void*)&mem);
  pzclSetKernelArg(kernel,  1, sizeof(int),    (void*)&index);

  {
    size_t global_work_size = 128;
    if ((result = pzclEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL)) != PZCL_SUCCESS)
      Error("pzclEnqueueNDRangeKernel failed, %d\n", result);
  }

  result = pzclEnqueueReadBuffer(queue, mem, PZCL_TRUE, 0, sizeof(PZCPerformance), &perf, 0, NULL, NULL);
  if (result != PZCL_SUCCESS) Error("pzclEnqueueReadBuffer failed %d\n", result);

  return perf;
}

void PrintProfileData(pzcl_context context, pzcl_command_queue queue, pzcl_kernel kernel, int nTrial) {
  const double clock = 733e6;
  FILE* prof;
  char* fn = new char[256];
  sprintf(fn, "profile.trial%d.log", nTrial);
  prof = fopen(fn, "w");
  fprintf(prof, "%16s\t%27s\t%19s\t%19s\n", "SECTION", "PERFORMANCE", "STALL", "WAIT");
  for (int i = 0; i < MAX_PROF; i++) {
    PZCPerformance perf = GetPerformance(context, queue, kernel, i);
    unsigned int perf_count = perf.Perf();
    unsigned int stall      = perf.Stall();
    unsigned int wait       = perf.Wait();
    double sec  = perf_count / clock;
    switch (i) {
      case UPDATE_GR:      fprintf(prof, "%16s\t", "Update GR"); break;
      case CALC_INPUT_PKJ: fprintf(prof, "%16s\t", "Calc Input PKJ"); break;
      case CALC_INPUT_ST:  fprintf(prof, "%16s\t", "Calc Input ST"); break;
      case UPDATE_GO:      fprintf(prof, "%16s\t", "Update GO"); break;
      case UPDATE_MOL:     fprintf(prof, "%16s\t", "Update MOL"); break;
      case ALL:            fprintf(prof, "%16s\t", "ALL"); break;
      default:             fprintf(prof, "%16s\t", "Undef"); break;
    }
    fprintf(prof, "%8d (%e sec)\t", perf_count, (perf_count != 0) ? sec : 0.0);
    fprintf(prof, "%8d (%6.3f %%)\t", stall, (perf_count != 0) ? stall * 100.0 / perf_count: 0.0 );
    fprintf(prof, "%8d (%6.3f %%)\n", wait , (perf_count != 0) ? wait * 100.0 / perf_count: 0.0 );
  }
  delete fn;
  fclose(prof);
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

  pzcl_kernel kernel_ltd = pzclCreateKernel(program, KERNEL_LTD, &err);
  if (err) Error("pzclCreateKernel of %s: %d\n", KERNEL_LTD, err);

  pzcl_kernel kernel_init = pzclCreateKernel(program, KERNEL_INIT, &err);
  if (err) Error("pzclCreateKernel of %s: %d\n", KERNEL_INIT, err);

  pzcl_kernel kernel_fin  = pzclCreateKernel(program, KERNEL_FIN, &err);
  if (err) Error("pzclCreateKernel of %s: %d\n", KERNEL_FIN, err);

#ifdef PROF
  pzcl_kernel kernel_prof = pzclCreateKernel(program, KERNEL_PROF, &err);
  if (err) Error("pzclCreateKernel of %s: %d\n", KERNEL_PROF, err);
#endif

  pzcl_command_queue queue = pzclCreateCommandQueue(context, device[device_id], 0, &err);
  if (err) Error("pzclCreateCommandQueue: %d\n", err);

  // Stackサイズを1.2KBに
  SetStackSize (kernel,      STACK_SIZE);
  SetStackSize (kernel_ltd,  STACK_SIZE);
  SetStackSize (kernel_init, STACK_SIZE);
  SetStackSize (kernel_fin,  STACK_SIZE);
#ifdef PROF
  SetStackSize (kernel_prof, STACK_SIZE);
#endif

  //
  // Data Setup
  //

  unsigned long* s_gr_all = (unsigned long*)malloc(sizeof(unsigned long) * N_S_GR * 9);
  unsigned long* s_gr     = &s_gr_all[N_S_GR * 4];
  if (s_gr_all == NULL) Error("failed malloc s_gr_all\n");
  for (int i = 0; i < N_S_GR * 9; i++) s_gr_all[i] = 0;
  pzcl_mem dev_s_gr = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned long) * N_S_GR, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_gr\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_gr, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR, s_gr, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_gr\n", err);

  unsigned long* s_gr_rect = (unsigned long*)malloc(sizeof(unsigned long) * N_S_GR * 3);
  if (s_gr_rect == NULL) Error("failed malloc s_gr_rect\n");
  for (int i = 0; i < N_S_GR * 3; i++) s_gr_rect[i] = 0;
  pzcl_mem dev_s_gr_rect = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned long) * N_S_GR * 3, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_gr_rect\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_gr_rect, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR * 3, s_gr_rect, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_gr_rect\n", err);

  unsigned char* s_go = (unsigned char*)malloc(sizeof(unsigned char) * N_GO);
  if (s_go == NULL) Error("failed malloc s_go\n");
  for (int i = 0; i < N_GO; i++) s_go[i] = 0;
  pzcl_mem dev_s_go = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned char) * N_GO, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_go\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_go, PZCL_TRUE, 0, sizeof(unsigned char) * N_GO, s_go, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_go\n", err);

  unsigned char* s_mol = (unsigned char*)malloc(sizeof(unsigned char) * N_MOL);
  if (s_mol == NULL) Error("failed malloc s_mol\n");
  for (int i = 0; i < N_MOL; i++) s_mol[i] = 0;
  pzcl_mem dev_s_mol = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned char) * N_MOL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_s_mol\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_s_mol, PZCL_TRUE, 0, sizeof(unsigned char) * N_MOL, s_mol, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d s_mol\n", err);

  srand(23L);
  unsigned int* seeds = (unsigned int*)malloc(sizeof(unsigned int) * WORK_UNIT_SIZE);
  if (seeds == NULL) Error("failed malloc seeds\n");
  for (int i = 0; i < WORK_UNIT_SIZE; i++) seeds[i] = rand();
  pzcl_mem dev_seeds = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned int) * WORK_UNIT_SIZE, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_seeds\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_seeds, PZCL_TRUE, 0, sizeof(unsigned int) * WORK_UNIT_SIZE, seeds, 0, NULL, NULL);
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
    float e_leak = (  GR(i) * (E_LEAK_GR) 
                    + GO(i) * (E_LEAK_GO)
                    +PKJ(i) * (E_LEAK_PKJ)
                    + ST(i) * (E_LEAK_ST)
                    + VN(i) * (E_LEAK_VN)
                    + IO(i) * (E_LEAK_IO));
    u[i] = e_leak;
  }
  pzcl_mem dev_u = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_u\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_u, PZCL_TRUE, 0, sizeof(float) * N_ALL, u, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d u\n", err);

  float* g_ex = (float*)malloc(sizeof(float) * N_ALL);
  if (g_ex == NULL) Error("failed malloc g_ex\n");
  for (int i = 0; i < N_ALL; i++) g_ex[i] = 0.0f;
  pzcl_mem dev_g_ex = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_g_ex\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_g_ex, PZCL_TRUE, 0, sizeof(float) * N_ALL, g_ex, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d g_ex\n", err);

  float* g_inh = (float*)malloc(sizeof(float) * N_ALL);
  if (g_inh == NULL) Error("failed malloc g_inh\n");
  for (int i = 0; i < N_ALL; i++) g_inh[i] = 0.0f;
  pzcl_mem dev_g_inh = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * N_ALL, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_g_inh\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_g_inh, PZCL_TRUE, 0, sizeof(float) * N_ALL, g_inh, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d g_inh\n", err);

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

  float* debug = (float*)malloc(sizeof(float) * WORK_UNIT_SIZE);
  if (debug == NULL) Error("failed malloc debug\n");
  for (int i = 0; i < WORK_UNIT_SIZE; i++) debug[i] = 0.0f;
  pzcl_mem dev_debug = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * WORK_UNIT_SIZE, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_debug\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_debug, PZCL_TRUE, 0, sizeof(float) * WORK_UNIT_SIZE, debug, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d debug\n", err);

  float* sum = (float*)malloc(sizeof(float) * 2048);
  if (sum == NULL) Error("failed malloc sum\n");
  for (int i = 0; i < 2048; i++) sum[i] = 0.0f;
  pzcl_mem dev_sum = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(float) * 2048, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_sum\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_sum, PZCL_TRUE, 0, sizeof(float) * 2048, sum, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d sum\n", err);

  unsigned int* w = (unsigned int*)malloc(sizeof(unsigned int) * (N_GR / 4) * N_PKJ);
  if (w == NULL) Error("failed malloc w\n");
  for (int i = 0; i < (N_GR / 4) * N_PKJ; i++) w[i] = 0xffffffff;
  pzcl_mem dev_w = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned int) * (N_GR / 4) * N_PKJ, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_w\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_w, PZCL_TRUE, 0, sizeof(unsigned int) * (N_GR / 4) * N_PKJ, w, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d w\n", err);

  unsigned int head = 0;
  unsigned long* mem_s_gr = (unsigned long*)malloc(sizeof(unsigned long) * N_S_GR * MEM_TIME);
  if (mem_s_gr == NULL) Error("failed malloc mem_s_gr\n");
  for (int i = 0; i < N_S_GR * MEM_TIME; i++) mem_s_gr[i] = 0;
  pzcl_mem dev_mem_s_gr = pzclCreateBuffer(context, PZCL_MEM_READ_WRITE, sizeof(unsigned long) * N_S_GR * MEM_TIME, NULL, &err);
  if (err) Error("pzclCreateBuffer:%d dev_mem_s_gr\n", err);
  err = pzclEnqueueWriteBuffer(queue, dev_mem_s_gr, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR * MEM_TIME, mem_s_gr, 0, NULL, NULL);
  if (err) Error("pzclEnqueueWriteBuffer:%d w\n", err);

  //
  // Main Loop
  //
  
  char* h_spikep_buf = (char*)malloc(sizeof(char) * N_ALL_P * N_PERIOD);

  size_t work_unit_size = WORK_UNIT_SIZE;
  pzcl_event event = NULL;

  pzclSetKernelArg(kernel_init, 0, sizeof(pzcl_mem), &dev_seeds);
  pzclSetKernelArg(kernel_init, 1, sizeof(pzcl_mem), &dev_w);
  err = pzclEnqueueNDRangeKernel(queue, kernel_init, 1, NULL, &work_unit_size, NULL, 0, NULL, &event);
  if (err) Error("pzclEnqueueNDRangeKernel: %d\n", err);
  if (event) {
    err = pzclWaitForEvents(1, &event);
    if (err != PZCL_SUCCESS)
      Error("Err: kernel %d in begin. Rank %d on %s.\n", err, world_rank, processor_name);
  }

  MPI_Status status_mpi;
  MPI_Request request[16];
  int dst_rank, src_rank, rank_list[8];

  int size = 2;
  int row = world_rank / size;
  int col = world_rank % size;

  rank_list[0] = ((col - 1 + size) % size) + size * ((row - 1 + size) % size); //UpperLeft
  rank_list[1] = ( col                   ) + size * ((row - 1 + size) % size); //Upper
  rank_list[2] = ((col + 1       ) % size) + size * ((row - 1 + size) % size); //UpperRight
  rank_list[3] = ((col - 1 + size) % size) + size * ( row                   ); //Left
  rank_list[4] = ((col + 1       ) % size) + size * ( row                   ); //Right
  rank_list[5] = ((col - 1 + size) % size) + size * ((row + 1       ) % size); //LowerLeft
  rank_list[6] = ( col                   ) + size * ((row + 1       ) % size); //Lower
  rank_list[7] = ((col + 1       ) % size) + size * ((row + 1       ) % size); //LowerRight

#ifdef DEBUG
  FILE* local_fin;
  if (world_rank == 0) {
    char fn[256];
    sprintf(fn, "local_fin.log");
    local_fin = fopen(fn, "w");
  }
#endif

  for (int nt = 0; nt < N_TRIALS; nt++) {
    FILE* fgr_spk;
    FILE* log;
    if (world_rank == 0) {
      char fn[1024];
#ifdef PRINT
      sprintf(fn, "gr.spk.%d.%d", world_rank, nt);
      fgr_spk = fopen(fn, "w");
#endif
#ifdef DEBUG
      sprintf(fn, "log.%d.%d", world_rank, nt);
      log = fopen(fn, "w");
#endif
      fprintf(stderr, "== %2d ==\n", nt);
    }

    pzclSetKernelArg(kernel,  0, sizeof(pzcl_mem), &dev_lists);
    pzclSetKernelArg(kernel,  1, sizeof(pzcl_mem), &dev_s_gr);
    pzclSetKernelArg(kernel,  2, sizeof(pzcl_mem), &dev_s_gr_rect);
    pzclSetKernelArg(kernel,  3, sizeof(pzcl_mem), &dev_s_go);
    pzclSetKernelArg(kernel,  4, sizeof(pzcl_mem), &dev_s_mol);
    pzclSetKernelArg(kernel,  5, sizeof(pzcl_mem), &dev_u);
    pzclSetKernelArg(kernel,  6, sizeof(pzcl_mem), &dev_g_ex);
    pzclSetKernelArg(kernel,  7, sizeof(pzcl_mem), &dev_g_inh);
    pzclSetKernelArg(kernel,  8, sizeof(pzcl_mem), &dev_g_ahp);
    pzclSetKernelArg(kernel,  9, sizeof(pzcl_mem), &dev_sum);
    pzclSetKernelArg(kernel, 10, sizeof(pzcl_mem), &dev_w);
    pzclSetKernelArg(kernel, 11, sizeof(pzcl_mem), &dev_spikep_buf);
    pzclSetKernelArg(kernel, 12, sizeof(pzcl_mem), &dev_debug);

    if (world_rank == 0) timer_start();

    int count = 0;
    for (int t_e = 0; t_e < N_PERIOD; t_e += T_I) {
      count++;

      pzclSetKernelArg(kernel, 13, sizeof(int), &t_e);

      // Gr spikes are exchaged
      //for (int rb = 0; rb < 2; rb++) {
      //  if ((row + col) % 2 == rb) {
      //    for (int nn = 0; nn < 8; nn++) {
      //      if (0 <= nn && nn <= 2) {
      //        MPI_Isend(&s_gr[N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn]);
      //      }
      //      if (3 == nn || nn == 4) {
      //        MPI_Isend(s_gr, N_S_GR, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn]);
      //      }
      //      if (5 <= nn && nn <= 7) {
      //        MPI_Isend(s_gr, N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn]);
      //      }
      //    }
      //  } else {
      //    for (int nn = 0; nn < 8; nn++) {
      //      if (0 <= nn && nn <= 2) {
      //        MPI_Irecv(&s_gr_all[nn * N_S_GR + N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn + 8]);
      //      }
      //      if (nn == 3) {
      //        MPI_Irecv(&s_gr_all[nn * N_S_GR], N_S_GR, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn + 8]);
      //      }
      //      if (nn == 4) {
      //        MPI_Irecv(&s_gr_all[(nn + 1) * N_S_GR], N_S_GR, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn + 8]);
      //      }
      //      if (5 <= nn && nn <= 7) {
      //        MPI_Irecv(&s_gr_all[(nn + 1) * N_S_GR], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[nn], 1, MPI_COMM_WORLD, &request[nn + 8]);
      //      }
      //    }
      //  }
      //}

      err = pzclEnqueueWriteBuffer(queue, dev_s_gr_rect, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR * 3, s_gr_rect, 0, NULL, NULL);
      if (err) Error("pzclEnqueueWriteBuffer: %d\n", err);

      err = pzclEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_unit_size, NULL, 0, NULL, &event);
      if (err) Error("pzclEnqueueNDRangeKernel: %d\n", err);

      MPI_Sendrecv(    &s_gr[             N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[7], 1,
                   &s_gr_all[             N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[0], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(    &s_gr[             N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[6], 1,
                   &s_gr_all[N_S_GR     + N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[1], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(    &s_gr[             N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[5], 1,
                   &s_gr_all[N_S_GR * 2 + N_S_GR / 4 * 3], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[2], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(     s_gr                             , N_S_GR    , MPI_UNSIGNED_LONG, rank_list[4], 1,
                   &s_gr_all[N_S_GR * 3                 ], N_S_GR    , MPI_UNSIGNED_LONG, rank_list[3], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(     s_gr                             , N_S_GR    , MPI_UNSIGNED_LONG, rank_list[3], 1,
                   &s_gr_all[N_S_GR * 5                 ], N_S_GR    , MPI_UNSIGNED_LONG, rank_list[4], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(     s_gr                             , N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[2], 1,
                   &s_gr_all[N_S_GR * 6                 ], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[5], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(     s_gr                             , N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[1], 1,
                   &s_gr_all[N_S_GR * 7                 ], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[6], 1,
                   MPI_COMM_WORLD, &status_mpi);
      MPI_Sendrecv(     s_gr                             , N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[0], 1,
                   &s_gr_all[N_S_GR * 8                 ], N_S_GR / 4, MPI_UNSIGNED_LONG, rank_list[7], 1,
                   MPI_COMM_WORLD, &status_mpi);

      if (event) {
        err = pzclWaitForEvents(1, &event);
        if (err) Error("pzclWaitForEvents: %d in trial.\n", err);
      }

      err = pzclEnqueueReadBuffer(queue, dev_s_gr, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR, s_gr, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      
      // LTD
      err = pzclEnqueueReadBuffer(queue, dev_s_gr, PZCL_TRUE, 0, sizeof(unsigned long) * N_S_GR, mem_s_gr + (head * N_S_GR), 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer mem_s_gr : %d\n", err);
      head = (head + 1) % MEM_TIME;
      if ((nt + 1) % 10 == 0) {
        err = pzclEnqueueReadBuffer(queue, dev_s_mol, PZCL_TRUE, 0, sizeof(unsigned char) * N_MOL, s_mol, 0, NULL, NULL);
        if (err) Error("err: read from dev_s_mol\n");
        if (s_mol[IDX_H_IO - N_GR - N_GO]) {
          err = pzclEnqueueWriteBuffer(queue, dev_mem_s_gr, PZCL_TRUE, 0, N_S_GR*MEM_TIME*sizeof(unsigned long), mem_s_gr, 0, NULL, NULL);
          if (err) Error("EnqueueWriteBuffer: %d\n", err);
          pzclSetKernelArg(kernel_ltd, 0, sizeof(pzcl_mem), &dev_mem_s_gr);
          pzclSetKernelArg(kernel_ltd, 1, sizeof(unsigned int), &head);
          pzclSetKernelArg(kernel_ltd, 2, sizeof(pzcl_mem), &dev_w);
          if (world_rank == 0) { fprintf(stderr, "Enter LTD: buffer head is %d time is %d\n", head, t_e); }

          err = pzclEnqueueNDRangeKernel(queue, kernel_ltd, 1, NULL, &work_unit_size, NULL, 0, NULL, &event);
          if (err){ fprintf(stderr, "Err: EnqueueNDRangeKernel: %d\n", err); }
          if (event) {
            err = pzclWaitForEvents(1, &event);
            if (err) Error("Err: kernel %d in ltd.\n", err);
          } 
        }
      }

#ifdef PRINT
      err = pzclEnqueueReadBuffer(queue, dev_spikep_buf, PZCL_TRUE, 0, sizeof(char) * N_ALL_P * T_I, spikep_buf, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      for (int t_i = 0; t_i < T_I; t_i++) {
        int t_each = t_e + t_i;
        for (int i = 0; i < N_ALL_P; i++) {
          h_spikep_buf[t_each * N_ALL_P + i] = spikep_buf[t_i * N_ALL_P + i];
        }
      }
#endif

      // Wait data exchange
      //for (int i = 0; i < 16; i++) MPI_Wait(&request[i], &status_mpi);

      // Crop necessary region and rearrange
      for (int dx = -X_GO_4; dx < X_GO + X_GO_4; dx++) {
        int xx = (dx < 0) * (0) + (0 <= dx && dx < X_GO) * (1) + (dx >= X_GO) * (2);
        int ax = (dx + X_GO) % X_GO;
        int ax_rect = dx + X_GO_4;
        for (int dy = -Y_GO_4; dy < Y_GO + Y_GO_4; dy++) {
          int yy = (dy < 0) * (0) + (0 <= dy && dy < Y_GO) * (1) + (dy >= Y_GO) * (2);
          int ay = (dy + Y_GO) % Y_GO;
          int ay_rect = dy + Y_GO_4;
          int ii = xx * 3 + yy;
          int base = N_S_GR * ii;
          int idx = ax + X_GO * ay;
          int idx_rect = ax_rect + (X_GO + X_GO_2) * ay_rect;
          for (int z = 0; z < 16; z++)
            s_gr_rect[idx_rect * 16 + z] = s_gr_all[base + idx * 16 + z];
        }
      }
    }

    if (world_rank == 0) {
      double elapsed_time = timer_elapsed();
      fprintf(stderr, "time: %f msec\n", elapsed_time);

#ifdef PRINT
      for (int t = 0; t < N_PERIOD; t++) {
        for (int i = 0; i < N_ALL_P; i++)
          if (h_spikep_buf[t * N_ALL_P + i] != 0) fprintf(fgr_spk, "%d %d\n", t, i);
      }
      fclose(fgr_spk);
#endif
#ifdef PROF
      PrintProfileData(context, queue, kernel_prof, nt);
#endif
#ifdef DEBUG
      err = pzclEnqueueReadBuffer(queue, dev_w, PZCL_TRUE, 0, sizeof(unsigned int) * N_S_GR * 16, w, 0, NULL, NULL);
      if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
      for (int i = 0; i < N_S_GR * 16; i++)
        fprintf(log, "%x\n", w[i]);
      fclose(log);
#endif
    }
  }

  pzclSetKernelArg(kernel_fin, 0, sizeof(pzcl_mem), &dev_debug);
  err = pzclEnqueueNDRangeKernel(queue, kernel_fin, 1, NULL, &work_unit_size, NULL, 0, NULL, &event);
  if (err) Error("pzclEnqueueNDRangeKernel: %d\n", err);
  err = pzclEnqueueReadBuffer(queue, dev_debug, PZCL_TRUE, 0, sizeof(float) * WORK_UNIT_SIZE, debug, 0, NULL, NULL);
  if (err) Error("pzclEnqueueReadBuffer: %d\n", err);
#ifdef DEBUG
  if (world_rank == 0) {
    long loop_go = 0;
    //long loop_pkj = 0;
    for (int i = 0; i < WORK_UNIT_SIZE; i++) {
      if (i % 8 == 0) {
        //loop_pkj += (int)debug[i + 6];
        fprintf(local_fin, "%x\n", (unsigned int)debug[i + 6]);
        loop_go += (int)debug[i + 7];
      }
      fprintf(local_fin, "%f\n", debug[i]);
    }
    //fprintf(local_fin,"loop_pkj:%ld\n", loop_pkj);
    fprintf(local_fin,"loop_go:%ld\n", loop_go);
    fclose(local_fin);
  }
#endif

  // Release memory objects
  free(seeds);
  free(lists);
  free(s_gr_all);
  free(s_gr_rect);
  free(s_go);
  free(s_mol);
  free(u);
  free(g_ex);
  free(g_inh);
  free(g_ahp);
  free(sum);
  free(w);
  free(spikep_buf);
  free(debug);
  free(mem_s_gr);
  
  // Release PZCL Resources
  pzclReleaseEvent(event);
  pzclReleaseMemObject(dev_seeds);
  pzclReleaseMemObject(dev_lists);
  pzclReleaseMemObject(dev_s_gr);
  pzclReleaseMemObject(dev_s_gr_rect);
  pzclReleaseMemObject(dev_s_go);
  pzclReleaseMemObject(dev_s_mol);
  pzclReleaseMemObject(dev_u);
  pzclReleaseMemObject(dev_g_ex);
  pzclReleaseMemObject(dev_g_inh);
  pzclReleaseMemObject(dev_g_ahp);
  pzclReleaseMemObject(dev_sum);
  pzclReleaseMemObject(dev_w);
  pzclReleaseMemObject(dev_spikep_buf);
  pzclReleaseMemObject(dev_debug);
  pzclReleaseMemObject(dev_mem_s_gr);
  pzclReleaseCommandQueue(queue);
  pzclReleaseKernel(kernel);
  pzclReleaseKernel(kernel_ltd);
  pzclReleaseKernel(kernel_init);
  pzclReleaseKernel(kernel_fin);
  pzclReleaseKernel(kernel_prof);
  pzclReleaseProgram(program);
  pzclReleaseContext(context);

  // Finalize MPI environment
  MPI_Finalize();

  return 0;
}
