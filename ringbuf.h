#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

typedef float ValType ;
#define VALTYPE_MAX FLT_MAX;
#define VALTYPE_MIN FLT_MIN;

#define RINGBUF_CAP_INCREMENT 1024
typedef struct RingBuf RingBuf;
struct RingBuf{
	ValType* data;
	size_t capacity;
	size_t size;
	ValType* write_head;
	ValType* read_head;
};

void RingBuf_init(RingBuf* buf, size_t capacity);
void RingBuf_grow(RingBuf* buf);
void RingBuf_advance_head(RingBuf* buf, ValType** head);
void RingBuf_retreat_head(RingBuf* buf, ValType** head);
void RingBuf_print(RingBuf* buf);
void RingBuf_write(RingBuf* buf, ValType value);
ValType RingBuf_read(RingBuf* buf);
ValType RingBuf_get_max(RingBuf* buf);
ValType RingBuf_get_min(RingBuf* buf);

#ifdef RINGBUF_IMPLEMENTATION
void RingBuf_init(RingBuf* buf, size_t capacity){
	buf->data = (ValType*) malloc(sizeof(ValType)*capacity);
	if(!buf->data){
		fprintf(stderr, "I require more RAM!\n");
		exit(1);
	}
	buf->capacity = capacity;
	buf->size = 0;
	buf->write_head = buf->data;
	buf->read_head = buf->data;
	memset(buf->data,0,sizeof(ValType)*buf->capacity);
}

void RingBuf_grow(RingBuf* buf){
	buf->data = (ValType*) realloc(buf->data, sizeof(ValType)*(buf->capacity+RINGBUF_CAP_INCREMENT));
	if(!buf->data){
		fprintf(stderr, "I require more RAM!\n");
		exit(1);
	}
	buf->capacity += RINGBUF_CAP_INCREMENT;
}

void RingBuf_advance_head(RingBuf* buf, ValType** head){
	(*head)++;
	if(*head >= buf->data + buf->size){
		*head = buf->data;
	}
}

void RingBuf_retreat_head(RingBuf* buf, ValType** head){
	(*head)--;
	if(*head < buf->data){
		*head = buf->data + buf->size-1;
	}
}

void RingBuf_print(RingBuf* buf){
	printf("buf->size: %lu, buf->data: [ ",buf->size);
	for(size_t i = 0; i < buf->size; i++){
		if(buf->data+i == buf->write_head){
			printf("\033[42m%f\033[0m, ",buf->data[i]);
		}else{
			printf("%f, ",buf->data[i]);
		}
	}
	printf("]\n");
}

void RingBuf_write(RingBuf* buf, ValType value){
	if(buf->size < buf->capacity){
	buf->size++;
	}
	RingBuf_advance_head(buf, &buf->write_head);
	buf->write_head[0] = value;
}

ValType RingBuf_read(RingBuf* buf){
	ValType value = *buf->read_head;
	RingBuf_advance_head(buf, &buf->read_head);
	return value;
}

ValType RingBuf_get_max(RingBuf* buf){
	ValType max = VALTYPE_MIN;
	for(size_t i = 0; i < buf->size; i++){
		if(buf->data[i] > max){
			max = buf->data[i];
		}
	}
	return max;
}

ValType RingBuf_get_min(RingBuf* buf){
	ValType min = VALTYPE_MAX;
	for(size_t i = 0; i < buf->size; i++){
		if(buf->data[i] < min){
			min = buf->data[i];
		}
	}
	return min;
}
#endif
