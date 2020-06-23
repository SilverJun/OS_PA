#include <stdio.h>
#include <unistd.h>
#include "smalloc.h"

extern char etext, edata, end ; 
int 
main()
{
	void *p1, *p2, *p3, *p4, *p5, *p6 ;

	print_sm_containers() ;

	p1 = smalloc(2000) ; 
	printf("smalloc(2000):%p\n", p1) ; 
	print_sm_containers() ;

	p2 = smalloc(2500) ; 
	printf("smalloc(2500):%p\n", p2) ; 
	print_sm_containers() ;

	p3 = smalloc(1000) ; 
	printf("smalloc(1000):%p\n", p3) ; 
	print_sm_containers() ;

	p4 = smalloc(500) ; 
	printf("smalloc(1000):%p\n", p4) ; 
	print_sm_containers() ;

	// print_mem_uses();

	// p5 = smalloc(2500) ; 
	// p6 = smalloc(2500) ; 
	// printf("program break at :  %p\n", sbrk(0)) ;
	// sfree(p6);
	
	// sshrink();
	// printf("program break at :  %p\n", sbrk(0)) ;
	// print_sm_containers() ;
	// print_mem_uses();

	// srealloc(p1, 4000);
	// print_sm_containers() ;
	// print_mem_uses();
}
