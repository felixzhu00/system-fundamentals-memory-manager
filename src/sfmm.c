/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

#include <errno.h>// to get ENOMEM macro

//TODO: Check valid_pointer
//Make sure all the flags are right

void delete_flst(sf_block * ptr){
    ptr->body.links.prev->body.links.next = ptr->body.links.next;
    ptr->body.links.next->body.links.prev = ptr->body.links.prev;
    ptr->body.links.prev = NULL;
    ptr->body.links.next = NULL;}
void insert_flst(sf_block * ptr, sf_block * prev){
    ptr->body.links.next = prev->body.links.next;
    prev->body.links.next = ptr;
    ptr->body.links.prev = prev;
    ptr->body.links.next->body.links.prev = ptr;}
void insert_main_by_size(sf_block *ptr, size_t size){
    size_t lsb = ptr->header & 0x6;
    ptr->header = size | lsb;

    for(int i = 0, mult = 1; i < NUM_FREE_LISTS; i++, mult *=2){
        if(size <= mult*32 || (i==9)){
            insert_flst(ptr, &sf_free_list_heads[i]);
            break;
        }
    }}
void *check_flst_for_block(size_t alloc_size){
    //Check freelist for free block
    for(int i = 0, mult = 1; i < NUM_FREE_LISTS; i++, mult*=2){
        //Skip empty free lists
        if(sf_free_list_heads[i].body.links.next == &sf_free_list_heads[i]){
            continue;
        }
        //Check if the current free list is matching size
        if(alloc_size <= mult*32|| (i==9)){
            sf_block * current = sf_free_list_heads[i].body.links.next;
            while(current != &sf_free_list_heads[i]){
                //Get the first free block
                size_t temp_size = ((current->header & ~0x1) & ~0x2) & ~0x4;

                if(!(current->header & 0x1) & (temp_size > alloc_size)){
                    temp_size -= alloc_size;
                    //Pop free block out of list
                    delete_flst(current);
                    //Split the header
                    sf_block * rem_block = (void *)current + alloc_size;

                    //Footer
                    void * footer = ((((void *)rem_block) -8 + temp_size));
                    *(sf_footer*)footer = temp_size | 0x2;

                    //Insert both back in to the main freelist
                    insert_main_by_size(rem_block, temp_size);
                    // //Update header flags
                    size_t lsb = current->header & 0x7;
                    current->header = alloc_size | lsb;
                    current->header |= 0x1;// Call the delete function so that used block does not link
                    rem_block->header |= 0x2;
                    return (void *)current;
                }
                if(!(current->header & 0x1) & (temp_size == alloc_size)){
                    current->header |= 0x1;// Call the delete function so that used block does not link
                    current->body.links.next->header |= 0x2;
                    delete_flst(current);
                    return (void *)current;
                }
            current = current->body.links.next;
            }
        }
    }
    return NULL;}

void valid_pointer(void * pp){

    sf_header pp_header = ((sf_block*)pp)->header;

    if(pp == NULL || !((unsigned long)pp & 0x7) == 0){
        // printf("(A)");
        abort();
    }


    if((pp_header & ~0x7) < 32 || !((pp_header & ~0x7)%8 == 0)){
        // printf("(B)");
        abort();
    }


    if(pp < (sf_mem_start() + 32) || pp > (sf_mem_end()-8)){
        // printf("(C)");
        abort();
    }


    if((pp_header & 0x1) == 0 || ((pp_header & 0x4))){
        // printf("(D)");
        abort();
    }
    //might need to check this below
    if((pp_header & 0x2) == 0){
        printf("(E)");
        //loop through from all block to get to the block before current block
        void * mem_ptr = sf_mem_start()+32;
        void * prv_mem_ptr = sf_mem_start();
        while(mem_ptr < pp){
            size_t size = (*(sf_header*)mem_ptr & ~0x7);
            prv_mem_ptr = (void *)mem_ptr;
            mem_ptr += size;
        }
        //Check if this prev block has the alloc bit as 1, if so abort else its fine
        if((*(sf_header*)prv_mem_ptr & 0x1) == 1){
            printf("(E)");
            // abort();
        }

    }}
void coalesce(void * ptr){
    //Coalesce front and back
    sf_block * temp = ptr;

    sf_header * curr_header = ((void*)temp);
    sf_footer * curr_footer = (void *)curr_header + (*(curr_header) & ~0x7)- 8;

    sf_footer * prv_footer = (void*)curr_header - 8;
    sf_header * prv_header = (void*)prv_footer - ((*prv_footer & ~0x7) + 8);

    sf_header * next_header = (void*)curr_header + (*curr_header &~0x7);
    sf_footer * next_footer = (void *)next_header + (*next_header &~0x7) - 8;

    int prev = (*curr_header & 0x2); // 1 if previous is allocted
    int next = (*next_header & 0x1); // 1 if next is allocted

    //If both are not free
    if(prev && next){
        //Return itself
        *curr_header = (*curr_header &~0x5);
        *curr_footer = *curr_header;
        insert_main_by_size((void*)curr_header, (*curr_header&~0x7));

        //set alloc bit of header after that
        sf_header * next_next_header = (void*)curr_header + (*curr_header &~0x7);
        *next_next_header &= ~0x2;

        if(!(*next_next_header & 0x1)){
            sf_footer * next_next_footer = (void *)next_next_header + (*next_next_header &~0x7) - 8;
            *next_next_footer = *next_next_header;
        }
    }
    //If next is free
    if(prev && !next){
        //curr_header with next_footer
        *curr_header += ((*next_header) &~0x7);
        *curr_header &= ~0x5;
        *next_footer = *curr_header;

        //if next is in main list, pop it out
        if(!(*next_header&0x4)){
            delete_flst((void *)next_header);
        }

        //set alloc bit of header after that
        sf_header * next_next_header = (void*)curr_header + (*curr_header &~0x7);
        *next_next_header &= ~0x2;

        if(!(*next_next_header & 0x1)){
            sf_footer * next_next_footer = (void *)next_next_header + (*next_next_header &~0x7) - 8;
            *next_next_footer = *next_next_header;
        }

        insert_main_by_size((void*)curr_header, (*curr_header&~0x7));


    }
    //If prev is free
    if(!prev && next){
        //prv_header with curr_footer
        *prv_header += (*curr_header &~0x7);
        *prv_header &= ~0x5;
        *curr_footer = *prv_header;

        //if prv is in main list, pop it out
        if(!(*prv_header&0x4)){
            delete_flst((void *)prv_header);
        }

        //set alloc bit of header after that
        sf_header * next_next_header = (void*)prv_header + (*prv_header &~0x7);
        *next_next_header &= ~0x2;

        if(!(*next_next_header & 0x1)){
            sf_footer * next_next_footer = (void *)next_next_header + (*next_next_header &~0x7) - 8;
            *next_next_footer = *next_next_header;
        }

        insert_main_by_size((void*)prv_header, (*prv_header&~0x7));
    }
    //If both are free
    if(!prev && !next){
        //prv_header with next_footer
        *prv_header += ((*curr_header &~0x7) + (*next_header &~0x7)) ;
        *prv_header &= ~0x5;
        *next_footer = *prv_header;

        //if either is in main list, pop it out
        if(!(*prv_header&0x4)){
            delete_flst((void *)prv_header);
        }
        if(!(*next_header&0x4)){
            delete_flst((void *)next_header);
        }

        //set alloc bit of header after that
        sf_header * next_next_header = (void*)prv_header + (*prv_header &~0x7);
        *next_next_header &= ~0x2;

        if(!(*next_next_header & 0x1)){
            sf_footer * next_next_footer = (void *)next_next_header + (*next_next_header &~0x7) - 8;
            *next_next_footer = *next_next_header;
        }

        insert_main_by_size((void*)prv_header, (*prv_header&~0x7));
    }}

void *sf_malloc(size_t size) {
//TODO use 0x1 and 0x2 and 0x4 variables instead
    //Check for invalid size
    if (size == 0){
        return NULL;
    }

    // //Initialization: Main freelist "Dummies"
    if(sf_free_list_heads[0].body.links.prev == NULL && sf_free_list_heads[0].body.links.next == NULL){
        for(int i = 0; i < NUM_FREE_LISTS; i++){
            if(sf_free_list_heads[i].body.links.prev != &sf_free_list_heads[i]){
                sf_free_list_heads[i].header = 32;
                void * head_footer = &sf_free_list_heads[i].header + 32 - 8;
                *((sf_footer*)head_footer) = 32;
                sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
                sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            }
        }
    }
    //Initialization: Heap
    if(sf_mem_start() == sf_mem_end()){
        void * init = sf_mem_grow();
        if(init == NULL)
            return NULL;

        // //Provide initial padding for alignment
        int init_add = (uintptr_t) init;
        init_add = (init_add%8) == 0 ? 0 : ((8-(init_add%8)));
        init = init + init_add;

        // //Create Prologue
        sf_block * prologue = (sf_block *)init;
        prologue->header = 32;
        prologue->header |= 0x1;

        // //Create Epilogue
        sf_block * epilogue = (init + 4096 - 8);
        epilogue->header = 0;
        epilogue->header |= 0x1;

        //Insert remainder into freelist
        int remainder = 4096-32-8;
        sf_block * new_block = ((void *)prologue + 32);
        void * footer = ((((void *)new_block) + (remainder -8)));
        *(int *)footer = remainder | 0x2;

        new_block->header |= 0x2;
        insert_main_by_size(new_block, remainder);
    }

    //Compute the size needed to be allocated
    size_t block_size = size + 8;
    size_t align_size = (block_size%8) == 0 ? block_size : ((8-(block_size%8) + block_size));
    size_t alloc_size = align_size < 32 ? 32 : align_size;

    //Search quicklist for matching size
    size_t s = 32;
    for(int i = 0; i< NUM_QUICK_LISTS; i++, s+=8){
        if((sf_quick_lists[i].length) > 0){
            //Check if the size match
            if(s == alloc_size){
                //Found matching length

                //Pop the quicklist to main list
                sf_block * temp = sf_quick_lists[i].first;
                sf_block * temp_next = temp->body.links.next;

                sf_quick_lists[i].first = temp_next;
                sf_quick_lists[i].length--;

                //Set header bits
                temp->header &= ~0x4;

                //return block
                return ((void *)temp+8);
                break;
            }
        }
    }

    void * freelst_flag = NULL;

    //Aquire free block from freelist

    while(!(freelst_flag = check_flst_for_block(alloc_size))){
        //Check the end of the free list
        void * end = sf_mem_end();

        //Epilogue
        sf_header * epilogue = (sf_mem_end() - 8);

        //Preset value change later base on branch
        void * temp_footer = end - 16; //This is assuming footer exist

        int temp_size = 0;
        void * temp_header = NULL;//Points to either the header or the epilogue

        //Check if block before epilogue is allocated
        //Check if footer is valid
        if(((*epilogue & PREV_BLOCK_ALLOCATED) == 0) && ((*(sf_footer *)temp_footer & IN_QUICK_LIST) == 0) && ((*(sf_footer*)temp_footer & ~0x7) != 0)){
            //There is a free block before Epilogue
            //Compute header location base on footer
            temp_size = (((*(sf_footer*)temp_footer & ~0x1) & ~0x2) & ~0x4);
            temp_header = temp_footer - temp_size + 8;
            //Pop free block from freelist
            delete_flst((sf_block *)temp_header);
        }else{
            //There is no free block before Epilogue
            //Since footer does not exist user Epilogue as new sf_header
            temp_header = temp_footer - temp_size + 8;
            (*(sf_header *)temp_header) += 8;
        }

        //Get sf_header of free block(without 3 bit mask)
        (*(sf_header *)temp_header) = (((*(sf_header*)temp_header & ~0x1) & ~0x2) & ~0x4);

        int too_larg_flag = 0;
        //Grew heap to size that can fit alloc request
        while((*(sf_header *)temp_header)  < alloc_size){
            void * addr = sf_mem_grow();
            if(addr == NULL){
                too_larg_flag =1;
                break;
            }
            (*(sf_header *)temp_header) += PAGE_SZ;
        }

        void * new_footer = (void *)temp_header + (*(sf_header *)temp_header) - 8;
        *((sf_footer*)new_footer) = *((sf_header *)temp_header) ;
        //Create new Epilogue
        epilogue = (sf_mem_end() - 8);
        *epilogue = 0;
        *epilogue |= 0x1;

        insert_main_by_size((sf_block *)temp_header, *((sf_header *)temp_header));
        if(too_larg_flag){
            sf_errno = ENOMEM;
            return NULL;
        }
    }

    return (((sf_block*)freelst_flag)->body.payload);
}

void sf_free(void *pp) {
//TODO might want to coalesce pp when adding into the
//pp might be the payload address so a bit calculation might be needed
    // TO BE IMPLEMENTED

    //Check if pointer is valid
    valid_pointer(pp-8);

    //Get size to see what quicklist to insert it to
    sf_header pp_header = ((sf_block*)(pp-8))->header;
    size_t temp_size = (pp_header&~0x7);

    //Check if quicklist exist of size
    int found_flag = 0;
    int i = 0;
    size_t s = 32;

    for(; i< NUM_QUICK_LISTS; i++, s+=8){
        if(temp_size == s){
            found_flag =1;
            break;
        }
    }

    //Branch if quicklist is found
    if(found_flag){
        sf_block * pp_block = (sf_block *)(pp-8);

        //Set the header flag
        pp_block->header |= 0x4;

        if(sf_quick_lists[i].length == QUICK_LIST_MAX){

            //coalesce each node in link list
            while(sf_quick_lists[i].first != NULL){
            // for(int j = 0; j< QUICK_LIST_MAX; j++){
                sf_block * temp = sf_quick_lists[i].first;
                sf_block * temp_next = temp->body.links.next;
                coalesce(temp);
                sf_quick_lists[i].first = temp_next;
                sf_quick_lists[i].length--;
            }
        }
        //Append to the head of the quicklist
        pp_block->body.links.next = sf_quick_lists[i].first;
        sf_quick_lists[i].first = pp_block;
        sf_quick_lists[i].length++;
    }else{
        coalesce(pp-8);
    }

    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    
    //check pp same as free
    valid_pointer(pp-8);

    //rsize == 0;
    if(rsize == 0){
        return NULL;
    }


    sf_block * pp_block = (sf_block *)(pp-8);

    size_t csize = (pp_block->header) & ~0x7;
    size_t lsb = (pp_block->header) & 0x7;

    if(csize > rsize){
        //reloc to a smaller block

        //Compute the size needed to be allocated
        size_t block_size = rsize + 8;
        size_t align_size = (block_size%8) == 0 ? block_size : ((8-(block_size%8) + block_size));
        size_t alloc_size = align_size < 32 ? 32 : align_size;

        //if spliting would result in a splinter
        if((csize - rsize) < 32){
            pp_block->header = csize | lsb;
        }else{
            //Break the block in two
            pp_block->header = alloc_size | lsb;

            sf_block * rem_block = (void *)pp_block + alloc_size;
            rem_block->header = (csize - alloc_size) | 0x2;

            sf_footer * rem_footer = (void *)rem_block + ((csize - alloc_size) -8);
            *rem_footer = rem_block->header;
            coalesce(rem_block); //does the insertion too
        }

        //will not result in a splinter

        return ((void *)pp_block+8);

    }else if(csize < rsize){
        void * result = sf_malloc(rsize);
        if(result != NULL){
            memcpy(result, pp, sizeof(pp_block->body.payload)+1);
            sf_free(pp);
        }
        return (result);
    }else{
        return pp;
    }

    // TO BE IMPLEMENTED
    return NULL;
    abort();
}

void *sf_memalign(size_t size, size_t align) {

    //Check align is at least as large as
    if(align < 8){
        sf_errno = EINVAL;
        return NULL;
    }

    //Check align is power of 2;
    int power_flag = 0; // 0 if not power of 2, else 1

    int temp_align = align;

    while(temp_align != 0){
        temp_align >>= 1;
        if((temp_align) == 2){
            power_flag = 1;
            break;
        }
    }
    if(!power_flag){
        sf_errno = EINVAL;
        return NULL;
    }

    size_t block_size = size + align + 32 + 16;
    size_t align_size = (block_size%8) == 0 ? block_size : ((8-(block_size%8) + block_size));
    size_t alloc_size = align_size < 32 ? 32 : align_size;

    //Aquire free block from freelist
    void * freelst_flag = sf_malloc(alloc_size-8);

    if(freelst_flag == NULL){
        return NULL;
    }

    freelst_flag = freelst_flag - 8;

    //Block of what malloc returns
    sf_header * block_header = freelst_flag;
    sf_footer * block_footer = freelst_flag + (*block_header&~0x7)-8;

    //Insert malloc block and convert it to a free block
    insert_main_by_size(freelst_flag, (*block_header)&~0x7);
    *block_footer = (*block_header);

    //normal payload address of block satisfy the requested alignment
    if(freelst_flag != NULL){
        if((((unsigned long)(freelst_flag + 8)) & (align-1)) == 0){
            //If payload is aligned
            //Split block if no splinter

            //Compute the size needed to be allocated 8 bit allignment
            size_t b_size = size + 8; //header
            size_t al_size = (b_size%8) == 0 ? b_size : ((8-(b_size%8) + b_size));
            size_t ac_size = al_size < 32 ? 32 : al_size;//size of block going to be return to user

            if((alloc_size - size > 32)){

                // Set new header and footer for block
                *block_header = ac_size | (*block_header&0x7);
                *block_header |= 0x1;
                block_footer = (void *)block_header + (*block_header&~0x7);
                *block_footer = *block_header;

                sf_block * rem_block = freelst_flag + ac_size;
                //Set header and footer of rem block
                rem_block->header = (alloc_size - ac_size) | 0x2;
                sf_footer * footer = ((((void *)rem_block) -8 + ((rem_block->header)&~0x7)));
                *footer = rem_block->header;

                //Coalesce the remainder before insert(coalesce should involve insert)
                coalesce(rem_block);
            }
            delete_flst(freelst_flag);
            ((sf_block *)freelst_flag)->header |= 0x1;
            // printf("\n\n\n(%li %li %li)\n\n\n", (uintptr_t)freelst_flag, ac_size,);
            return (freelst_flag + 8);
        }else{
            //Set header of splits to not allocated

            //Compute the new block address base on wanted alignment
            size_t payload_addr = ((unsigned long)(freelst_flag+8));//Mem address in long
            //add this to block to get new aligned payload address
            size_t round_size = (payload_addr%align) == 0 ? 0 : (align-(payload_addr%align));

            while(round_size < 32 || (payload_addr + round_size)% align != 0){
                //add 32(min_size) because split the front will result in a splinter
                if(round_size < 32){
                    //add 32 so that there is at least 32 when spliting
                    round_size += 32;
                }else{
                    //adjust round size to meet the alignment requirement
                    round_size += ((payload_addr + round_size)%align) == 0 ? 0 : (align-((payload_addr + round_size)%align));
                }
            }
            
            delete_flst(freelst_flag);

            size_t val_of_free = ((((sf_block *)freelst_flag)->header)&~0x7);// Total size after malloc

            //coalese the prev block
            sf_header * prev_block_header = freelst_flag;
            *prev_block_header = round_size | ((*(sf_header *)freelst_flag)&0x7);
            sf_footer * prev_block_footer = (void *)prev_block_header + ((*prev_block_header)&~0x7) - 8;
            *prev_block_footer = *prev_block_header;

            //Compute the size needed to be allocated 8 bit allignment
            size_t b_size = size + 8; //header
            size_t al_size = (b_size%8) == 0 ? b_size : ((8-(b_size%8) + b_size));
            size_t ac_size = al_size < 32 ? 32 : al_size;//size of block going to be return to user

            //Set footer and header of return block
            sf_header * new_block_header = freelst_flag + round_size;
            sf_footer * new_block_footer = (void *)new_block_header + (ac_size) -8;

            *new_block_header = (ac_size) | 0x1;
            *new_block_footer = *new_block_header;

            //Check if no splinter will be made if you free some padding of return block
            if(!((val_of_free - ac_size) < 32)){
                //will not result in a splinter

                //split the block
                sf_header * next_block_header = (void *)(new_block_header) + ac_size;
                sf_footer * next_block_footer = (void *)next_block_header + (val_of_free-round_size-ac_size)-8;

                //set prv alloc
                *next_block_header = (val_of_free-round_size-ac_size) | 0x2;
                *next_block_footer = *next_block_header;

                coalesce((void *)next_block_header);
            }

            return (((sf_block*)new_block_header)->body.payload);
        }
    }
    return NULL;
    abort();
}