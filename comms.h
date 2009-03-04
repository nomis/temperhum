void temper_delay(int n);
void temper_DTR(int set);
void temper_RTS(int set);
int temper_CTS(void);
void temper_clock(int a);
void temper_out(int a);
int temper_in(void);
void temper_clock_signal(void);
void Start_IIC(void);
void Stop_IIC(void);
void WriteP1P0(int P0123, char *dataS);
void temper_open(char *dev);

