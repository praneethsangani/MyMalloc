#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"

// Don't change or remove these constants.
#define MINIMUM_ALLOCATION  16
#define SIZE_MULTIPLE       8

typedef struct Block
{
	unsigned int size; 
	int in_Use; //1 = in use, 0 = not in use
	struct Block* next_Block; 
	struct Block* prev_Block; 

} Block;

Block* head = NULL; 
Block* tail = NULL; 
Block* last_allocated = NULL; 

void print_linkedList()
{
	Block* curr = head; 
	for(curr = head; curr != NULL; curr = curr->next_Block)
	{
		printf("[Size: %d, In use: %d] -> ", curr->size, curr->in_Use);
	}
}

int next_Block_Is_Null(Block* block)
{
	if(block->next_Block == NULL)
		return 1;
	return 0;
}

Block* increment_Block(Block* block)
{
	return block->next_Block; 
}

int the_Last_Allocated_Block_Is_The_Tail()
{
	if(last_allocated == tail)
		return 1; 
	return 0;
}
unsigned int round_up_size(unsigned int data_size)
{
	if(data_size == 0)
		return 0;
	else if(data_size < MINIMUM_ALLOCATION)
		return MINIMUM_ALLOCATION;
	else
		return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

Block* make_Initial_Block(unsigned int size)
{
	Block* block = sbrk(sizeof(Block) + size);
	block->size = size; 
	block->in_Use = 1;
	block->next_Block = NULL; 
	block->prev_Block = NULL; 
	head = block; 
	tail = block;
	last_allocated = block;
	return block + 1;
}

Block* sbrk_Block_And_Link(unsigned int size)
{
	Block* block = sbrk(sizeof(Block) + size);
	block->size = size; 
	block->in_Use = 1;
	block->next_Block = NULL; 
	tail->next_Block = block;
	block->prev_Block = tail; 
	tail = block;
	last_allocated = block;
	return block + 1;
}

Block* set_NextFit_Block(Block* block, unsigned int size)
{
	block->in_Use = 1; 
	block->size = size; 
	last_allocated = block;
	return block; 
}

int block_Is_Available(Block* block, unsigned int size)
{
	if((block->in_Use == 0)	&& (block->size >= size))
		return 1;
	return 0; 
}

Block* search_For_Free_Block(Block* block, unsigned int size)
{
	while(block != last_allocated)
	{
		if(block_Is_Available(block, size))
			return set_NextFit_Block(block, size);;	
		if(next_Block_Is_Null(block))
			block = head; 
		else
			block = increment_Block(block); 
	}
	return NULL; 
}

int head_Is_Available_And_Is_Last_Allocated_Block(Block* block, unsigned int size)
{
	if(head == last_allocated)
	{
		if(block_Is_Available(block, size))
			return 1;
	}
	return 0;
}

Block* nextfit_Block(unsigned int size)
{
	Block* block = head;
	if(head_Is_Available_And_Is_Last_Allocated_Block(block, size))
		return block;	
	if(!the_Last_Allocated_Block_Is_The_Tail())
		block = increment_Block(last_allocated);
	return search_For_Free_Block(block, size);
}

int last_allocated_Is_Null()
{
	if(last_allocated == NULL)
		return 1; 
	return 0; 
}

Block* allocate_Block(unsigned int size)
{
	Block* block = nextfit_Block(size);

	if(block != NULL)
		return block + 1;
	else
		return sbrk_Block_And_Link(size);
}

void* my_malloc(unsigned int size)
{
	
	if(size == 0)
		return NULL;
	size = round_up_size(size);
	//List is empty
	if(last_allocated_Is_Null())
		return make_Initial_Block(size); 
	else
	{
		return allocate_Block(size);
	}
}

int ptr_Is_Null(void* ptr)
{
	if(ptr == NULL)
		return 1;
	return 0; 
}

Block* make_Block_from_pointer(void* ptr)
{
	return (Block*)ptr - 1;
}

void set_Block_free(Block* block)
{
	block->in_Use = 0; 
}

void check_If_Block_Is_Last_Allocated_Block(Block* block)
{
	if(block == last_allocated)
		last_allocated = head; 
}

void set_Globals_To_Null()
{
	head = NULL;
	tail = NULL;
	last_allocated = NULL;
}

void decrement_tail(Block* block)
{
	tail = block->prev_Block;
	tail->next_Block = NULL; 
}

void move_brk_Down_If_Needed(Block* block)
{
	if(next_Block_Is_Null(block))
	{
		brk(block);
		if(head == tail)
			set_Globals_To_Null();
		else
			decrement_tail(block);
	}
}

int next_Block_Is_Free(Block* block)
{
	Block* next = block->next_Block; 
	if(next != NULL && next->in_Use == 0)
		return 1; 
	return 0;
}

int add_Block_Sizes(Block* block, Block* next_Block)
{
	return block->size + next_Block->size + sizeof(Block);
}

void link_Blocks_Forward(Block* block, Block* next_Block)
{
	block->next_Block = next_Block->next_Block;
}

int next_Block_Is_Tail(Block* next_Block)
{
	if(next_Block == tail)
		return 1; 
	return 0; 
}

void link_Blocks_Backwards(Block* block, Block* next_Next_Block)
{
	next_Next_Block->prev_Block = block;
}

Block* coalesce(Block* block)
{
	Block* next_Block = block->next_Block;
	Block* next_Next_Block = next_Block->next_Block;

	block->size = add_Block_Sizes(block, next_Block);
	link_Blocks_Forward(block, next_Block); 

	if(next_Block_Is_Tail(next_Block))
		tail = block; 
	else
		link_Blocks_Backwards(block, next_Next_Block);

	return block; 
}

int prev_Block_Is_Free(Block* block)
{
	Block* prev = block->prev_Block; 
	if(prev != NULL && prev->in_Use == 0)
		return 1; 
	return 0;
}

Block* coalesce_Blocks(Block* block)
{	
	if(next_Block_Is_Free(block))
		block = coalesce(block); 
	if(prev_Block_Is_Free(block))
		block = coalesce(block->prev_Block);
	return block; 
}

void my_free(void* ptr)
{
	if(ptr_Is_Null(ptr))
		return;

	Block* block = make_Block_from_pointer(ptr); 
	set_Block_free(block);
	block = coalesce_Blocks(block);
	check_If_Block_Is_Last_Allocated_Block(block);
	move_brk_Down_If_Needed(block);
}
