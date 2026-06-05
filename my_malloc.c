#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEAP_SIZE (1024 * 1024) // 1 MB
#define ALIGNMENT 16

typedef struct{
    size_t size;
    bool free;
} Chunk;

typedef struct {
    size_t size;
} Footer;

typedef struct FreeNode {
    struct FreeNode *next;
    struct FreeNode *prev;
} FreeNode;

Chunk* heap_head = NULL;

FreeNode* free_head = NULL;


int heap_init(){
    heap_head = (Chunk*)VirtualAlloc(NULL, HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (heap_head == NULL){
        return -1;
    }

    heap_head->size = HEAP_SIZE - sizeof(Chunk) - sizeof(Footer);
    heap_head->free = true;
    Footer* footer = get_footer(heap_head);
    footer->size = heap_head->size;


    free_head = (FreeNode*)(heap_head + 1);
    free_head->next = NULL;
    free_head->prev = NULL;

    return 0;
}

static size_t align_size(size_t size){
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

static Footer* get_footer(Chunk *chunk){
    return (Footer *)((char *)(chunk + 1) + chunk->size);
}

static void insert_free_node(FreeNode* node){
    node->next = free_head;
    node->prev = NULL;

    if (free_head){
        free_head->prev = node;
    }

    free_head = node;
}

static void remove_free_node(FreeNode* node){
    if (node->prev){
        node->prev->next = node->next;
    }
    else{
        free_head = node->next;
    }

    if (node->next){
        node->next->prev = node->prev;
    }
}

static void replace_free_node(FreeNode* old, FreeNode* new){
    new->prev = old->prev;
    new->next = old->next;

    if (new->prev){
        new->prev->next = new;
    }
    else{
        free_head = new;
    }

    if (new->next){
        new->next->prev = new;
    }
}

static Chunk* next_chunk(Chunk* chunk){
    return (Chunk*)((char*)get_footer(chunk) + sizeof(Footer));
}

static Chunk* prev_chunk(Chunk* chunk){
    Footer* prev_footer = (Footer*)((char*)chunk - sizeof(Footer));
    return (Chunk*)((char*)chunk - sizeof(Chunk) - sizeof(Footer) - prev_footer->size);
}

void* heap_alloc(size_t size) {
    size = align_size(size);

    FreeNode* current = free_head;

    while(current != NULL){
        Chunk* chunk = ((Chunk*)current) - 1;

        if (chunk->free && chunk->size >= size){
            chunk->free = false;

            if (chunk->size >= size + sizeof(Chunk) + sizeof(Footer) + sizeof(FreeNode)){
                Chunk* new_chunk = (Chunk*)((char*)(chunk + 1) + size);
                size_t old_size = chunk->size;

                new_chunk->size = old_size - size - sizeof(Chunk) - sizeof(Footer);
                new_chunk->free = true;

                Footer* new_footer = get_footer(new_chunk);
                new_footer->size = new_chunk->size;

                FreeNode* new_node = (FreeNode*)(new_chunk + 1);
                
                replace_free_node(current, new_node);

                chunk->size = size;

                Footer* footer = get_footer(chunk);
                footer->size = chunk->size;

                return (void*)(chunk + 1);
            }

            else{
                remove_free_node(current);
                
                return (void*)(chunk + 1);
            }
        }
        
        current = current->next;
    }

    return NULL;

}

void heap_free(void* ptr) {
    if(ptr == NULL){
        return;
    }

    Chunk* chunk = ((Chunk*)ptr) - 1;

    if(chunk->free){
        printf("Double free detected\n");
        return;
    }
    chunk->free = true;

    FreeNode* node = (FreeNode*)(chunk + 1);
    insert_free_node(node);

}


int main(){

    printf("%zu\n", sizeof(Chunk));

    return 0;
}