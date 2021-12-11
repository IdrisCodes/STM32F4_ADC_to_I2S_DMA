#ifndef AUDIO_H_
#define AUDIO_H_



#define AUDIO_I2C_ADDRESS (0x94U)
#define AUDIO_FREQUENCY (8000U)
#define DECIMATION_FACTOR (64U)
#define N_PDM_1BIT_SAMPLES_PER_MS (AUDIO_FREQUENCY*DECIMATION_FACTOR)




void Audio_Init(void);
void Audio_Loop(void);












#endif /* AUDIO_H_ */
