#ifdef COMMS_C

/* Input */
int temper_in(void);

/* Output */
void temper_clock(int v);
void temper_clock_signal(void);
void temper_out(int v);

/* Timing */
void temper_delay(int n);

#endif

/* Comms */
int temper_read(void);
void temper_switch(int rising, int falling);
int temper_wait(int timeout);
void temper_write(int data, int len);
// TODO remove
int temper_get(void);

/* Device */
void temper_open(char *dev);
void temper_close(void);
