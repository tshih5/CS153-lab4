#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
	acquire(&(shm_table.lock));
	
	int i = 0;
	for(i = 0; i < 64; ++i){
		if(id == shm_table.shm_pages[i].id){		//if the id matches in the page table
			//mape the virtual address to the physical address
			mappages(myproc()->pgdir, (char*)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
			//increment refcount
			shm_table.shm_pages[i].refcnt++;
			//update pointer
			*pointer = (char*)PGROUNDUP(myproc()->sz);
			//update process size			
			myproc()->sz = PGROUNDUP(myproc()->sz + PGSIZE);
		}else{										//does not match
			//initialize id in table
			shm_table.shm_pages[i].id = id;
			//allocate memory for the frame
			shm_table.shm_pages[i].frame = kalloc();
			//clear memory allocated by kalloc()
			memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
			//set reference counter
			shm_table.shm_pages[i].refcnt = 1;
			//map virtual address to physical address
			mappages(myproc()->pgdir, (char*)PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
			//update pointer
			*pointer = (char*)PGROUNDUP(myproc()->sz);
			//update process size
			myproc()->sz = PGROUNDUP(myproc()->sz) + PGSIZE;
		}
	}
	release(&(shm_table.lock));



return 0; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
	acquire(&(shm_table.lock));
	int i = 0;
	for(i = 0; i < 64; ++i){
		if(shm_table.shm_pages[i].id == id){
			if(shm_table.shm_pages[i].refcnt != 1){
				shm_table.shm_pages[i].refcnt--;
			}else if (shm_table.shm_pages[i].refcnt == 1){
				//it it refcount is the last count
				//clear table
				shm_table.shm_pages[i].id =0;
				shm_table.shm_pages[i].frame =0;
				shm_table.shm_pages[i].refcnt =0;
			}
		}	
	}
	release(&(shm_table.lock));
return -1; //added to remove compiler warning -- you should decide what to return
}
