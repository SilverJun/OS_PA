#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include "smalloc.h" 

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

// TODO : First fit -> Best fit
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


void print_mem_uses()
{

}
