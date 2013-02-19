#include <kern/ts7200.h>

int getRegister(int base, int offset);

int getRegisterBit(int base, int offset, int mask);

void setRegister(int base, int offset, int value);

void setRegisterBit(int base, int offset, int mask, int value);

unsigned int setTimerLoadValue(int timer_base, unsigned int value);

unsigned int setTimerControl(int timer_base, unsigned int enable, unsigned int mode, unsigned int clksel);

unsigned int getTimerValue(int timer_base);

unsigned int setUARTLineControl(int uart_base, unsigned int word_len,
                                unsigned int fifo, unsigned int two_stop,
                                unsigned int even_parity, unsigned int parity,
                                unsigned int brk, unsigned int speed);

unsigned int setUARTControl(int uart_base, unsigned int enable,
                            unsigned int loopback, unsigned int recv_timeout,
                            unsigned int trans_irq, unsigned int recv_irq,
                            unsigned int ms_irq);

unsigned int setUARTControlBit(int uart_base, unsigned int mask, int value);

unsigned int enableVicInterrupt(int vic_base, int mask);
