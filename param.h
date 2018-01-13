#define R_N2 (32 * 32)

#define N_GR (32 * 32 * R_N2)

#define N_ALL ((N_GR))

#define N_S_GR (N_GR / 64)

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

#define N_MF_PER_GR     (4)
#define GBAR_EX_GR      (0.18f)
#define E_EX_GR         (0.0f)
#define DECAY_AMPA_GRMF (0.43460f) //exp(-(float)DT/tau_ampa_grmf);
#define DECAY_NMDA_GRMF (0.98095f) //exp(-(float)DT/tau_nmda_grmf);
#define R_AMPA_GRMF     (0.88f)
#define R_NMDA_GRMF     (0.12f)
#define LAMBDA          (16.0f)

#define IDX_H_GR (0)
#define IDX_T_GR ((IDX_H_GR) + (N_GR))

#define GR(i) (((IDX_H_GR) <= (i)) && ((i) < (IDX_T_GR)))

#define N_TRIALS (1)
#define N_PERIOD (6000) // 6000 msec
#define INV_N_PERIOD (0.00016666f) // 1/6000
#define T_I (4)
#define DT (1.0f)

#define WORK_UNIT_SIZE (16384)//(15872) //1984PE * 8Thread
#define N_MAX_THREADS (WORK_UNIT_SIZE)

#define N_GR_P (128)
#define N_ALL_P ((N_GR_P))
