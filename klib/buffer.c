#include <klib.h>

void cBufferInitial(CharBuffer *char_buffer, char *buffer, int buffer_size) {
	char_buffer->buffer = buffer;
	char_buffer->front = 0;
	char_buffer->back = 0;
	char_buffer->current_size = 0;
	char_buffer->buffer_size = buffer_size;
}

void cBufferPush(CharBuffer *char_buffer, char c) {
	assert(char_buffer->current_size < char_buffer->buffer_size, "IOBuffer exceed buffer size");
	char_buffer->buffer[char_buffer->back] = c;
	char_buffer->back = (char_buffer->back + 1) % char_buffer->buffer_size;
	char_buffer->current_size += 1;
}

char cBufferPop(CharBuffer *char_buffer) {
	assert(char_buffer->current_size > 0, "IOBuffer has no char left to be popped");
	char ret = char_buffer->buffer[char_buffer->front];
	char_buffer->front = (char_buffer->front + 1) % char_buffer->buffer_size;
	char_buffer->current_size -= 1;
	return ret;
}

void iBufferInitial(IntBuffer *int_buffer, int *buffer, int buffer_size) {
	int_buffer->buffer = buffer;
	int_buffer->front = 0;
	int_buffer->back = 0;
	int_buffer->current_size = 0;
	int_buffer->buffer_size = buffer_size;
}

void iBufferPush(IntBuffer *int_buffer, int i) {
	assert(int_buffer->current_size < int_buffer->buffer_size, "IOBuffer exceed buffer size");
	int_buffer->buffer[int_buffer->back] = i;
	int_buffer->back = (int_buffer->back + 1) % int_buffer->buffer_size;
	int_buffer->current_size += 1;
}

int iBufferPop(IntBuffer *int_buffer) {
	assert(int_buffer->current_size > 0, "IOBuffer has no char left to be popped");
	char ret = int_buffer->buffer[int_buffer->front];
	int_buffer->front = (int_buffer->front + 1) % int_buffer->buffer_size;
	int_buffer->current_size -= 1;
	return ret;
}


