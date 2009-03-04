/* Input */
int temper_read(void);
// TODO remove
int temper_get(void);

/* Output */
void temper_pause(void);
void temper_out(int v);

/* Timing */
void temper_delay(int n);

/* Comms */
void temper_switch(int rising, int falling);
int temper_wait(int timeout);
void temper_write(int data, int len);

/* Device */
void temper_open(char *dev);
void temper_close(void);
