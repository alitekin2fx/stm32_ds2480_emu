#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

struct ring_buffer
{
	int size;
	void *storage;
	__IO int end_pos;
	__IO int start_pos;
};

void init_ring_buffer(struct ring_buffer *buffer, void *storage, int size);
void ring_buffer_write(struct ring_buffer *buffer, const void *data, int data_length);

__STATIC_INLINE void ring_buffer_set_empty(struct ring_buffer *buffer)
{
	buffer->start_pos = buffer->end_pos;
}

__STATIC_INLINE int ring_buffer_is_empty(struct ring_buffer *buffer)
{
	return(buffer->start_pos == buffer->end_pos);
}

__STATIC_INLINE int ring_buffer_is_full(struct ring_buffer *buffer)
{
	return(buffer->start_pos == ((buffer->end_pos + 1) % buffer->size));
}


#endif /* __RING_BUFFER_H__ */
