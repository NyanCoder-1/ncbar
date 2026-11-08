#ifndef APP_SIGNALS_H
#define APP_SIGNALS_H

int app_signals_block(void);    // call before creating any threads
int app_signals_open_fd(void);  // signalfd, or -1 on error
void app_signals_drain(int fd); // consume pending signal records

#endif