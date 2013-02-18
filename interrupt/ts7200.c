#include <ts7200.h>
#include <klib.h>

int getRegister(int base, int offset) {
	int *addr = (int *)(base + offset);
	return *addr;
}

int getRegisterBit(int base, int offset, int mask) {
	return (getRegister(base, offset)) & mask;
}

void setRegister(int base, int offset, int value) {
	int *addr = (int *)(base + offset);
	*addr = value;
}

void setRegisterBit(int base, int offset, int mask, int value) {
	int buf = getRegister(base, offset);
	buf = value ? buf | mask : buf & ~mask;
	setRegister(base, offset, buf);
}

unsigned int setTimerLoadValue(int timer_base, unsigned int value) {
	unsigned int* timer_load_addr = (unsigned int*) (timer_base + LDR_OFFSET);
	*timer_load_addr = value;
	return *timer_load_addr;
}

unsigned int setTimerControl(int timer_base, unsigned int enable, unsigned int mode, unsigned int clksel) {
	unsigned int* timer_control_addr = (unsigned int*) (timer_base + CRTL_OFFSET);
	unsigned int control_value = (ENABLE_MASK & enable) | (MODE_MASK & mode) | (CLKSEL_MASK & clksel) ;

	*timer_control_addr = control_value;
	return *timer_control_addr;
}

unsigned int getTimerValue(int timer_base) {
	unsigned int* timer_value_addr = (unsigned int*) (timer_base + VAL_OFFSET);
	unsigned int value = *timer_value_addr;
	return value;
}

unsigned int setUARTLineControl(int uart_base, unsigned int word_len,
                                unsigned int fifo, unsigned int two_stop,
                                unsigned int even_parity, unsigned int parity,
                                unsigned int brk, unsigned int speed) {

	// Set the Baud Rate Divisor
	unsigned int *speed_high_addr = (unsigned int *)( uart_base + UART_LCRM_OFFSET );
	unsigned int *speed_low_addr = (unsigned int *)( uart_base + UART_LCRL_OFFSET );
	switch( speed ) {
		case 115200:
			*speed_high_addr = 0x0;
			*speed_low_addr = 0x3;
			break;
		case 2400:
			*speed_high_addr = 0x0;
			*speed_low_addr = 0xbf;
			break;
		default:
			return -1;
	}

	// Set the line control register high
	unsigned int* uart_line_high_addr = (unsigned int*) (uart_base + UART_LCRH_OFFSET);
	unsigned int control_value = (FEN_MASK & fifo) |
								 (STP2_MASK & two_stop) |
								 (EPS_MASK & even_parity) |
								 (PEN_MASK & parity) |
								 (BRK_MASK & brk);
	control_value |= ((word_len << 5) & WLEN_MASK);

	*uart_line_high_addr = control_value;
	return *uart_line_high_addr;
}

unsigned int setUARTControl(int uart_base, unsigned int enable,
                            unsigned int loopback, unsigned int recv_timeout,
                            unsigned int trans_irq, unsigned int recv_irq,
                            unsigned int ms_irq) {
	unsigned int* uart_control_addr = (unsigned int*) (uart_base + UART_CTLR_OFFSET);
	unsigned int control_value = (UARTEN_MASK & enable) |
								 (LBEN_MASK & loopback) |
								 (RTIEN_MASK & recv_timeout) |
								 (TIEN_MASK & trans_irq) |
								 (RIEN_MASK & recv_irq) |
								 (MSIEN_MASK & ms_irq);

	*uart_control_addr = control_value;
	return *uart_control_addr;
}

unsigned int setUARTControlBit(int uart_base, unsigned int mask, int value) {
	unsigned int* uart_control_addr = (unsigned int*) (uart_base + UART_CTLR_OFFSET);
	*uart_control_addr = value ? *uart_control_addr | mask : *uart_control_addr & ~mask;
	return *uart_control_addr;
}

unsigned int enableVicInterrupt(int vic_base, int mask) {
	unsigned int* vic_enable_addr = (unsigned int*) (vic_base + VIC_IN_EN_OFFSET);
	*vic_enable_addr = (*vic_enable_addr) | mask;
	DEBUG(DB_IRQ, "| IRQ:\tEnabled with addr 0x%x and flag 0x%x", vic_enable_addr, *vic_enable_addr);
	return *vic_enable_addr;
}
