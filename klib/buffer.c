#include <klib.h>

void bufferInitial(CircularBuffer *circ_buffer, BufferType type, void *buffer, int buffer_size) {
	circ_buffer->type = type;
	circ_buffer->buffer.voids = buffer;
	circ_buffer->front = 0;
	circ_buffer->back = 0;
	circ_buffer->current_size = 0;
	circ_buffer->buffer_size = buffer_size;
}

void bufferPush(CircularBuffer *circ_buffer, int value) {
	assert(circ_buffer->current_size < circ_buffer->buffer_size, "IOBuffer exceed buffer size");
	switch(circ_buffer->type) {
		case CHARS:
			circ_buffer->buffer.chars[circ_buffer->back] = (char)(value);
			break;
		case INTS:
			circ_buffer->buffer.ints[circ_buffer->back] = (int)(value);
			break;
		case POINTERS:
			circ_buffer->buffer.pointers[circ_buffer->back] = (void *)(value);
			break;
		case VOIDS:
		default:
			assert(0, "Unkown or VOIDS buffer type is not allowed");
			break;
	}
	circ_buffer->back = (circ_buffer->back + 1) % circ_buffer->buffer_size;
	circ_buffer->current_size += 1;
}

int bufferPop(CircularBuffer *circ_buffer) {
	assert(circ_buffer->current_size > 0, "IOBuffer has no char left to be popped");
	int ret = -1;
	switch(circ_buffer->type) {
		case CHARS:
			ret = (int) circ_buffer->buffer.chars[circ_buffer->front];
			break;
		case INTS:
			ret = (int) circ_buffer->buffer.ints[circ_buffer->front];
			break;
		case POINTERS:
			ret = (int) circ_buffer->buffer.pointers[circ_buffer->front];
			break;
		case VOIDS:
		default:
			assert(0, "Unkown or VOIDS buffer type is not allowed");
			break;
	}
	circ_buffer->front = (circ_buffer->front + 1) % circ_buffer->buffer_size;
	circ_buffer->current_size -= 1;
	return ret;
}

void bufferPushStr(CircularBuffer *circ_buffer, char *str, int strlen) {
	assert(circ_buffer->current_size + strlen < circ_buffer->buffer_size, "IOBUFFER exceed buffer size(pushs)");
	// todo
}

void bufferPushChar(CircularBuffer *circ_buffer, char c) {
	circ_buffer->buffer.chars[circ_buffer->back] = c;
	circ_buffer->back = (circ_buffer->back + 1) % circ_buffer->buffer_size;
	circ_buffer->current_size += 1;
}