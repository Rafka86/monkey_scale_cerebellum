#define R_N2 (32 * 32)

#define N_GR (32 * 32 * R_N2)
#define X_GO (32)
#define Y_GO (32)
#define N_GO (1024) // X_GO * Y_GO

#define N_ALL ((N_GR) + (N_GO))

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
#define N_GO_PER_GR     (8)

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
#define I_EX_GO      (0.0f)

#define N_GR_PER_GO      (4)
#define DECAY_AMPA_GOGR  (0.51342f) //exp(-(float)DT/tau_ampa_gogr);
#define DECAY_AMPA_GOGR2 (0.26360f) //DECAY_AMPA_GOGR*DECAY_AMPA_GOGR
#define DECAY_AMPA_GOGR3 (0.13534f) //DECAY_AMPA_GOGR2*DECAY_AMPA_GOGR
#define DECAY_NMDA1_GOGR (0.96826f) //exp(-(float)DT/tau1_nmda_gogr);
#define DECAY_NMDA2_GOGR (0.99413f) //exp(-(float)DT/tau2_nmda_gogr);
#define R_AMPA_GOGR      (0.8f)
#define R_NMDA1_GOGR     (0.066f)   //(0.2*0.33)
#define R_NMDA2_GOGR     (0.134f)   //(0.2*0.67)
#define KAPPA_GOGR       (4.0e-5f)

#define IDX_H_GR (0)
#define IDX_T_GR ((IDX_H_GR) + (N_GR))
#define IDX_H_GO (IDX_T_GR)
#define IDX_T_GO ((IDX_H_GO) + (N_GO))

#define GR(i) (((IDX_H_GR) <= (i)) && ((i) < (IDX_T_GR)))
#define GO(i) (((IDX_H_GO) <= (i)) && ((i) < (IDX_T_GO)))

#define N_TRIALS (1)
#define N_PERIOD (6000) // 6000 msec
#define INV_N_PERIOD (0.00016666f) // 1/6000
#define T_I (4)
#define DT (1.0f)

#define WORK_UNIT_SIZE (16384)//(15872) //1984PE * 8Thread
#define N_MAX_THREADS (WORK_UNIT_SIZE)

#define N_GR_P (128)
#define N_GO_P (N_GO)
#define N_ALL_P ((N_GR_P) + (N_GO_P))

#define T_GR_P (N_GR_P)
#define T_GO_P ((T_GR_P) + (N_GO_P))
