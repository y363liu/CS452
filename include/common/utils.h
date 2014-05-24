#ifndef __UTILS_H__
#define __UTILS_H__


#define NULL (void*)0


typedef struct Queue{
	void* head;	
	void* tail;		
}Queue;

void*
memcpy(void* src, void* dest, int size);

#endif
