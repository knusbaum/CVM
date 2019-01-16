
typedef struct {
    unsigned char *buff;
    size_t buff_len;
    unsigned char *buff_ptr;
} writable_buffer;

void createbuffer(writable_buffer *wb);
void destroybuffer(writable_buffer *wb);
void buffer_write_byte(writable_buffer *wb, unsigned char byte);
void buffer_write_little16(writable_buffer *wb, uint16_t val);
void buffer_write_little32(writable_buffer *wb, uint32_t val);
size_t buffer_offset(writable_buffer *wb);
