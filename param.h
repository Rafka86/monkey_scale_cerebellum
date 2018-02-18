#include "modes.h"

constexpr int R_N2 = 32 * 32;

#define X_GR (32)
#define Y_GR (32)
constexpr int N_GR = X_GR * Y_GR * R_N2;
#define X_GO  (32)
#define Y_GO  (32)
constexpr int N_GO = X_GO * Y_GO;
#define N_PKJ (32)
#define N_ST  (32)
#define N_VN  (1)
#define N_IO  (1)

constexpr int N_ALL = N_GR + N_GO + N_PKJ + N_ST + N_VN + N_IO;
constexpr int N_MOL = N_PKJ + N_ST + N_VN + N_IO;

constexpr int N_S_GR = N_GR >> 6;
constexpr int N_W_GR = N_GR >> 2;
constexpr int X_GO_4 = X_GO >> 2;
constexpr int Y_GO_4 = Y_GO >> 2;
constexpr int X_GO_2 = X_GO >> 1;
constexpr int Y_GO_2 = Y_GO >> 1;

#define TH_GR        (-35.0f)
#define C_GR         (3.1f)
#define INV_C_GR     (0.32258f)
#define R_GR         (2.3f)
#define GBAR_LEAK_GR (0.43f)
#define E_LEAK_GR    (-58.0f)
#define GBAR_AHP_GR  (1.0f)
#define E_AHP_GR     (-82.0f)
#define TAU_AHP_GR   (5.0f)
#define DECAY_AHP_GR (0.81873f)
#define I_EX_GR      (0.0f)

#define N_MF_PER_GR       (4)
#define GBAR_EX_GR        (0.18f)
#define E_EX_GR           (0.0f)
#define DECAY_AMPA_GRMF   (0.43460f) //exp(-(float)DT/tau_ampa_grmf);
#define DECAY_NMDA_GRMF   (0.98095f) //exp(-(float)DT/tau_nmda_grmf);
#define R_AMPA_GRMF       (0.88f)
#define R_NMDA_GRMF       (0.12f)
#define LAMBDA            (16.0f)
#define N_GO_PER_GR       (8)
#define GBAR_INH_GR       (0.028f)
#define E_INH_GR          (-82.0f)
#define DECAY_GABA1_GRGO  (0.86688f)
#define DECAY_GABA1_GRGO2 (0.75148f)
#define DECAY_GABA1_GRGO3 (0.65144f)
#define R_GABA1_GRGO      (0.43f)
#define GAMMA             (8.0f)

#define TH_GO        (-52.0f)
#define C_GO         (28.0f)
#define INV_C_GO     (0.035714f)
#define R_GO         (0.428f)
#define GBAR_LEAK_GO (2.3f) //2.3nS
#define E_LEAK_GO    (-55.0f)
#define GBAR_EX_GO   (45.5f)
#define E_EX_GO      (0.0f)
#define GBAR_INH_GO  (0.0f) // No inhibitory inputs
#define E_INH_GO     (E_LEAK_GO)
#define GBAR_AHP_GO  (20.0f) //4.2nS
#define DECAY_AHP_GO (0.81873f) //exp(-(float)DT/tau_ahp_go);
#define E_AHP_GO     (-72.7f)
#define I_EX_GO      (3.0f)

#define N_GR_PER_GO      (4)
#define DECAY_AMPA_GOGR  (0.51342f) //exp(-(float)DT/tau_ampa_gogr);
#define DECAY_AMPA_GOGR2 (0.26360f) //DECAY_AMPA_GOGR*DECAY_AMPA_GOGR
#define DECAY_AMPA_GOGR3 (0.13534f) //DECAY_AMPA_GOGR2*DECAY_AMPA_GOGR
#define DECAY_NMDA1_GOGR (0.96826f) //exp(-(float)DT/tau1_nmda_gogr);
#define DECAY_NMDA2_GOGR (0.99413f) //exp(-(float)DT/tau2_nmda_gogr);
#define R_AMPA_GOGR      (0.8f)
#define R_NMDA1_GOGR     (0.066f)   //(0.2*0.33)
#define R_NMDA2_GOGR     (0.134f)   //(0.2*0.67)
#define KAPPA_GOGR       (2.0e-5f)

#define TH_PKJ        (-55.0f)     // mV
#define C_PKJ         (106.0f)     // pF
#define INV_C_PKJ     (0.0094339f)
#define R_PKJ         (0.431f)     // GOhm
#define GBAR_LEAK_PKJ (4.37f)      // 4.37 nS
#define E_LEAK_PKJ    (-68.0f)     // mV
#define GBAR_EX_PKJ   (0.7f)       // nS
#define E_EX_PKJ      (0.0f)       // mV
#define GBAR_INH_PKJ  (1.0f)
#define E_INH_PKJ     (-75.0f)
#define GBAR_AHP_PKJ  (100.0f)     // nS
#define E_AHP_PKJ     (-70.0f)     //E_LEAK_PKJ
#define DECAY_AHP_PKJ (0.670320f)  //exp(-(double)DT/tau_ahp_pkj);
#define I_EX_PKJ      (250.0f)

#define DECAY_AMPA_PKJPF  (0.886493f) //exp(-(double)DT/tau_ampa_pkjpf);
#define DECAY_AMPA_PKJPF2 (0.785790f)
#define DECAY_AMPA_PKJPF3 (0.696668f)
#define KAPPA_PKJPF       (3.0e-5f)   //3.0e-4f //(0.1*0.003)
#define DECAY_GABA_PKJST  (0.904847f) //exp(-DT/tau_gaba_pkjst);
#define DECAY_GABA_PKJST2 (0.818748f)
#define DECAY_GABA_PKJST3 (0.740842f)
constexpr int GAMMA_PKJST = 0.333f * N_ST;

#define TH_ST        (-55.0f)     // mV
#define C_ST         (106.0f)     // pF
#define INV_C_ST     (0.0094339f)
#define R_ST         (0.431f)     // GOhm
#define GBAR_LEAK_ST (4.37f)      // 4.37 nS
#define E_LEAK_ST    (-68.0f)     // mV
#define GBAR_EX_ST   (0.7f)       // nS
#define E_EX_ST      (0.0f)       // mV
#define GBAR_AHP_ST  (100.0f)     // nS
#define E_AHP_ST     (-70.0f)     //E_LEAK_PKJ
#define DECAY_AHP_ST (0.670320f)  //exp(-(double)DT/tau_ahp_st);
#define I_EX_ST      (200.0f)

#define DECAY_AMPA_STPF (0.886493f) //exp(-DT/tau_ampa_stpf);
#define KAPPA_STPF      (1.5e-5f)

#define TH_VN        (-38.8f)     // mV
#define C_VN         (122.3f)     // microF
#define INV_C_VN     (0.0081766f)
#define R_VN         (0.61f)      // kOhm
#define GBAR_LEAK_VN (1.64f)      //(1.0/(R_VN))
#define E_LEAK_VN    (-56.0f)     // mV
#define GBAR_EX_VN   (50.0f)      //0.1
#define E_EX_VN      (0.0f)       // mV
#define GBAR_INH_VN  (30.0f)      //0.5//0.05
#define E_INH_VN     (-88.0f)     // mV
#define GBAR_AHP_VN  (50.0f)      //0.5
#define E_AHP_VN     (-70.0f)     // mV
#define DECAY_AHP_VN (0.81873f)   //exp(-DT/tau_ahp_cn);
#define I_EX_VN      (500.0f)

#define DECAY_AMPA_VNMF  (0.43460f) //exp(-(float)DT/tau_ampa_grmf);
#define DECAY_GABA_VNPKJ (0.904f)   //10.0 (exp (/ -1.0 10.0))
#define R_AMPA_VNMF      (0.33f)    //0.66f
#define GAMMA_VNPKJ      (0.275f)   //0.125f //(2.0/(N_PKJ))

#define TH_IO        (-50.0f)
#define C_IO         (1.0f)
#define INV_C_IO     (1.0f)
#define GBAR_LEAK_IO (0.015f)
#define E_LEAK_IO    (-60.0f)
#define GBAR_EX_IO   (0.1f) //0.07//70.0
#define E_EX_IO      (0.0f)
#define GBAR_INH_IO  (0.018f)
#define E_INH_IO     (-75.0f)
#define GBAR_AHP_IO  (1.0f)
#define E_AHP_IO     (-70.0f)
#define DECAY_AHP_IO (0.81873f) //exp(-DT/tau_ahp_io);
#define I_EX_IO      (0.0f)

#define DECAY_AMPA_IO (0.904837f)

constexpr int IDX_H_GR  = 0;
constexpr int IDX_T_GR  = IDX_H_GR + N_GR;
constexpr int IDX_H_GO  = IDX_T_GR;
constexpr int IDX_T_GO  = IDX_H_GO + N_GO;
constexpr int IDX_H_PKJ = IDX_T_GO;
constexpr int IDX_T_PKJ = IDX_H_PKJ + N_PKJ;
constexpr int IDX_H_ST  = IDX_T_PKJ;
constexpr int IDX_T_ST  = IDX_H_ST + N_ST;
constexpr int IDX_H_VN  = IDX_T_ST;
constexpr int IDX_T_VN  = IDX_H_VN + N_VN;
constexpr int IDX_H_IO  = IDX_T_VN;
constexpr int IDX_T_IO  = IDX_H_IO + N_IO;
constexpr int IDX_H_MOL = IDX_T_GO;

constexpr bool  GR(int i) { return IDX_H_GR  <= i && i <  IDX_T_GR; }
constexpr bool  GO(int i) { return IDX_H_GO  <= i && i <  IDX_T_GO; }
constexpr bool PKJ(int i) { return IDX_H_PKJ <= i && i < IDX_T_PKJ; }
constexpr bool  ST(int i) { return IDX_H_ST  <= i && i <  IDX_T_ST; }
constexpr bool  VN(int i) { return IDX_H_VN  <= i && i <  IDX_T_VN; }
constexpr bool  IO(int i) { return IDX_H_IO  <= i && i <  IDX_T_IO; }

#define N_TRIALS (1)
#define N_PERIOD (6000) // 6000 msec
#define INV_N_PERIOD (0.00016666f) // 1/6000
#define T_I (4)
#define DT (1.0f)

#define MAX_PID        (1984)
#define MAX_TID        (8)
constexpr int WORK_UNIT_SIZE = MAX_PID * MAX_TID;
#define N_MAX_THREADS  (WORK_UNIT_SIZE)

#define N_GR_P  (1024)
#define N_GO_P  (N_GO)
#define N_PKJ_P (N_PKJ)
#define N_ST_P  (N_ST)
#define N_VN_P  (N_VN)
#define N_IO_P  (N_IO)
#define N_ALL_P ((N_GR_P) + (N_GO_P) + (N_PKJ_P) + (N_ST_P) + (N_VN_P) + (N_IO_P))

#define T_GR_P  (N_GR_P)
#define T_GO_P  ((T_GR_P)  + (N_GO_P) )
#define T_PKJ_P ((T_GO_P)  + (N_PKJ_P))
#define T_ST_P  ((T_PKJ_P) + (N_ST_P) )
#define T_VN_P  ((T_ST_P)  + (N_VN_P) )
#define T_IO_P  ((T_VN_P)  + (N_IO_P) )
