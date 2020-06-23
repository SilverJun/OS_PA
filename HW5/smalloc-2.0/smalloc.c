#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "smalloc.h"
#include <errno.h>

/**
 * struct _sm_container_t {
 *     sm_container_status status ;
 *     struct _sm_container_t * next ;
 *     struct _sm_container_t * prev ;
 *     size_t dsize ;
 * } ;
 */

sm_container_t sm_head = {
	0,
	&sm_head, 
	&sm_head,
	0 
} ;

static 
void * 
_data (sm_container_ptr e)
{
	return ((void *) e) + sizeof(sm_container_t) ;
}

static 
void 
sm_container_split (sm_container_ptr hole, size_t size)
{
	sm_container_ptr remainder = (sm_container_ptr) (_data(hole) + size) ;

	remainder->dsize = hole->dsize - size - sizeof(sm_container_t) ;	
	remainder->status = Unused ;
	remainder->next = hole->next ;
	remainder->prev = hole ;
	hole->dsize = size ;
	hole->next->prev = remainder ;
	hole->next = remainder ;
}

static 
void * 
retain_more_memory (int size)
{
	sm_container_ptr hole ;
	int pagesize = getpagesize() ;
	int n_pages = 0 ;

	n_pages = (sizeof(sm_container_t) + size + sizeof(sm_container_t)) / pagesize  + 1 ;
	hole = (sm_container_ptr) sbrk(n_pages * pagesize) ;
	if (hole == 0x0)
		return 0x0 ;
	
	hole->status = Unused ;
	hole->dsize = n_pages * getpagesize() - sizeof(sm_container_t) ;
	return hole ;
}

sm_container_ptr
_merge(sm_container_ptr p1, sm_container_ptr p2) // merge p1 p2
{
	// merge to p1
	p1->dsize += p2->dsize + sizeof(sm_container_t);

	// link connect
	p2->next->prev = p1;
	p1->next = p2->next;
	return p1;
}

void * 
smalloc (size_t size) 
{
	sm_container_ptr best_hole = 0x0, itr = 0x0 ;
	const size_t alloc_size = size + sizeof(sm_container_t);
	size_t best_diff = INT_MAX;

	for (itr = sm_head.next ; itr != &sm_head ; itr = itr->next) {
		if (itr->status == Busy)
			continue ;
		if (itr->dsize == size) { // just fit!
			best_hole = itr ;
			break;
		}
		size_t diff = itr->dsize - alloc_size;
		if ((alloc_size < itr->dsize) && (diff < best_diff)) { // find best fit hole
			best_diff = diff;
			best_hole = itr ;
		}
	}
	if (best_hole == 0x0) {
		best_hole = retain_more_memory(size) ;
		if (best_hole == 0x0)
			return 0x0 ;
		best_hole->next = &sm_head ;
		best_hole->prev = sm_head.prev ;
		(sm_head.prev)->next = best_hole ;
		sm_head.prev = best_hole ;
	}
	if (size < best_hole->dsize) 
		sm_container_split(best_hole, size) ;
	best_hole->status = Busy ;
	return _data(best_hole) ;
}

void *
srealloc (void * p, size_t newsize)
{
	sm_container_ptr best_hole = 0x0, itr = 0x0, p_node = 0x0;
	// if p node can extends to newsize, extend it.
	for (itr = sm_head.next ; itr != &sm_head ; itr = itr->next) {
		if (_data(itr) == p) {
			p_node = itr;
			break;
		}
	}

	if (p_node == 0x0) return 0x0;
	if (p_node->dsize == newsize) return _data(p_node);

	if (p_node->dsize > newsize) { // shrink case. no need to additional allocation.
		sm_container_split(p_node, newsize);
		return _data(p_node);
	}
	const size_t more_mem = newsize - p_node->dsize;
	// printf("%lu\n", more_mem);
	if (p_node->next->status == Unused && p_node->next->dsize > more_mem) { // merge next unused page.
		//memset(p_node->next, 0, p_node->dsize + sizeof(sm_container_t));
		_merge(p_node, p_node->next);
		sm_container_split(p_node, newsize);
		return _data(p_node);
	}
	else {
		void* newp = smalloc(newsize); // malloc newsize memmory.
		memcpy(newp, p, newsize); // copy original data.
		sfree(p); // free old memmory
		return newp;
	}

	return 0x0;
}

void 
sfree (void * p)
{
	sm_container_ptr itr ;
	for (itr = sm_head.next ; itr != &sm_head ; itr = itr->next) {
		if (p == _data(itr)) {
			itr->status = Unused ;

			while(itr->prev->status != Busy) {
				itr = _merge(itr->prev, itr);
			}
			while(itr->next->status != Busy) {
				itr = _merge(itr, itr->next);
			}

			break ;
		}
	}
}

void sshrink()
{
	// sm_head의 prev 부터 시작해서 unused 카운트하고 brk로 프로그램 브레이크 지정.
	sm_container_ptr itr ;
	void* breakpoint = 0x0;

	for (itr = sm_head.prev; itr != &sm_head ; itr = itr->prev) {
		//printf("%p\n", _data(itr));
		//printf("%s, %lu\n", itr->status == Busy?"Busy":"Unused", itr->dsize);
		if (itr->status == Busy) break;
	}
	breakpoint = (void*)itr->next;
	// link to head.
	itr->next = &sm_head;
	sm_head.prev = itr;
	if (brk(breakpoint) != 0) { // 여기가 힙의 끝.
		fprintf(stderr, "sshrink errno: %d", errno);
	}
}

void 
print_sm_containers ()
{
	sm_container_ptr itr ;
	int i ;

	printf("==================== sm_containers ====================\n") ;
	for (itr = sm_head.next, i = 0 ; itr != &sm_head ; itr = itr->next, i++) {
		printf("%3d:%p:%s:", i, _data(itr), itr->status == Unused ? "Unused" : "  Busy") ;
		printf("%8d:", (int) itr->dsize) ;

		int j ;
		char * s = (char *) _data(itr) ;
		for (j = 0 ; j < (itr->dsize >= 8 ? 8 : itr->dsize) ; j++) 
			printf("%02x ", s[j]) ;
		printf("\n") ;
	}
	printf("\n") ;

}

/**
 * print these following informations into stderr
 * (1) the amount of memory retained by smalloc so far -> sum of all nodes
 * (2) the amount of memory allocated by smalloc at this moment -> sum of busy nodes dsize
 * (3) the amount of memory retained by smalloc but not currently allocated. -> sum of unused nodes dsize
 */
void print_mem_uses()
{
	size_t all_size = 0, busy_size = 0, unused_size = 0;

	sm_container_ptr itr ;
	for (itr = sm_head.next; itr != &sm_head ; itr = itr->next) {
		all_size += itr->dsize+sizeof(sm_container_t);

		if (itr->status == Busy) {
			busy_size += itr->dsize;
		}
		else {
			unused_size += itr->dsize;
		}
	}
	
	fprintf(stderr, "==================== sm_containers mem uses ====================\n");
	fprintf(stderr, "the amount of memory retained by smalloc so far: %lu bytes\n", all_size);
	fprintf(stderr, "the amount of memory allocated by smalloc at this moment: %lu bytes\n", busy_size);
	fprintf(stderr, "the amount of memory retained by smalloc but not currently allocated: %lu bytes\n", unused_size);
}
