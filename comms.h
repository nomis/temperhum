#ifdef COMMS_C

/* Input */
int temper_in(void);

/* Output */
void temper_clock(int v);
void temper_clocked_out(int rising, int falling);
void temper_out(int v);

/* Timing */
void temper_delay(int n);

#endif

/* Comms */
unsigned int temper_read(int n);
int temper_wait(int timeout);
void temper_write_simple(unsigned int data, int len);
void temper_write_complex(unsigned int data, int len);

/* Device */
void temper_open(char *dev);
void temper_close(void);
