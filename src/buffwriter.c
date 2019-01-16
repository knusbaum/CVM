#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "buffwriter.h"

void createbuffer(writable_buffer *wb) {
    wb->buff = NULL;
    wb->buff_len = 0;
    wb->buff_ptr = NULL;
}

void destroybuffer(writable_buffer *wb) {
    if(wb->buff)
        free(wb->buff);
    wb->buff = NULL;
    wb->buff_ptr = NULL;
}

void buffer_write_byte(writable_buffer *wb, unsigned char byte) {
    if(!wb->buff) {
        wb->buff_len = 1024;
        wb->buff = malloc(wb->buff_len);
        wb->buff_ptr = wb->buff;
    }
    else if(wb->buff_ptr - wb->buff >= wb->buff_len) {
        uint32_t datalen = wb->buff_ptr - wb->buff;
        wb->buff_len *= 2;
        wb->buff = realloc(wb->buff, wb->buff_len);
        wb->buff_ptr = wb->buff + datalen;
    }
    *wb->buff_ptr = byte;
    wb->buff_ptr++;
}

void buffer_write_little16(writable_buffer *wb, uint16_t val) {
    int i;
    for(i = 0; i < 2; i++) {
        buffer_write_byte(wb, (val >> i * 8) & 0xFF);
    }
}

void buffer_write_little32(writable_buffer *wb, uint32_t val) {
    int i;
    for(i = 0; i < 4; i++) {
        buffer_write_byte(wb, (val >> i * 8) & 0xFF);
    }
}

size_t buffer_offset(writable_buffer *wb) {
    if(!wb) return 0;
    return wb->buff_ptr - wb->buff;
}
