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
