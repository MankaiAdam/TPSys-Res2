#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC_NUMBER 0xBADA110CBADA110CL
#define PREALLOC_SIZE 4096
#define VAR_SIZE_1 6
#define VAR_SIZE_2 12
#define VAR_SIZE_3 200

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next;
    size_t bloc_size;
    long magic_number;
} HEADER;

static HEADER* list = nullptr;

static void print()
{
    HEADER* it = list;

    while (it)
    {
        printf("%lu ->", it->bloc_size);
        it = it->ptr_next;
    }

    printf("\n");
}

static void split(HEADER* mem, size_t size)
{
    //+1 to increment by the size of the HEADER and cast the char to increment byte by byte
    long* footer = (long*)((char*)(mem + 1) + size);
    long magic = MAGIC_NUMBER;
    memcpy(footer, &magic, sizeof(long));

    HEADER* new_header = (HEADER*)(footer + 1);

    new_header->ptr_next = mem->ptr_next;
    new_header->bloc_size = mem->bloc_size - (size + sizeof(HEADER) + sizeof(long));
    new_header->magic_number = MAGIC_NUMBER;
    mem->bloc_size = size;
    mem->ptr_next = new_header;
}

static HEADER* get_bloc(size_t size)
{
    HEADER* it = list;
    HEADER* prev = nullptr;

    while (it)
    {
        if (it->bloc_size >= size)
        {
            size_t rem = it->bloc_size - size;
            if (rem > sizeof(HEADER) + sizeof(long))
                split(it,size);

            //remove bloc from list
            if (prev == NULL)
                list = it->ptr_next;
            else
                prev -> ptr_next = it->ptr_next;
            return it;
        }
        prev= it;
        it = it->ptr_next;
    }
    return nullptr;
}

static bool check_overflow(void* mem)
{
    HEADER* header = (HEADER*)mem;
    size_t size = header->bloc_size;

    if (header->magic_number != MAGIC_NUMBER)
    {
        fprintf(stderr, "Memory overflow detected!\n");
        return false;
    }

    long footer = *(long*)((char*)(header + 1) + size);

    if (footer != MAGIC_NUMBER)
    {
        fprintf(stderr, "Memory overflow detected!\n");
        return false;
    }
    return true;
}

static HEADER* bloc_end(HEADER* bloc)
{
    return (HEADER*)((long*)((char*)(bloc + 1) + bloc->bloc_size) + 1);
}

static HEADER* merge_blocs(HEADER* bloc1, HEADER* bloc2, bool left)
{
    if (left)
    {
        HEADER* it = list;
        HEADER* prev = nullptr;

        while (it && it != bloc2)
        {
            prev = it;
            it = it->ptr_next;
        }
        if (prev)
            prev->ptr_next = bloc1;
        else
            list = bloc1;
        bloc1->ptr_next = bloc2->ptr_next;
    }
    bloc1->bloc_size += bloc2->bloc_size + sizeof(HEADER) + sizeof(long);
    return bloc1;
}

static void insert_bloc(void* mem)
{
    HEADER* bloc = (HEADER*)mem;

    if (list == NULL)
    {
        bloc->ptr_next = nullptr;
        list = bloc;
        return;
    }

    HEADER* it = list;
    HEADER* prev = nullptr;

    //insert at start
    if (it > bloc)
    {
        if (bloc_end(bloc) == it)
        {
            bloc = merge_blocs(bloc, it, true);
            return;
        }
        bloc->ptr_next = it;
        list = bloc;
        return;
    }

    while (it && it < bloc)
    {
        prev = it;
        it = it->ptr_next;
    }

    bool merged = false;
    if (prev && bloc_end(prev) == bloc)
    {
        bloc = merge_blocs(prev, bloc, false);
        merged = true;
    }
    if (it && bloc_end(bloc) == it)
    {
        bloc = merge_blocs(bloc, it, true);
        merged = true;
    }
    if (!merged)
    {
        prev->ptr_next = bloc;
        bloc->ptr_next = it;
    }
}

void* malloc(size_t bloc_size)
{
    HEADER* available_bloc = get_bloc(bloc_size);

    if (available_bloc)
    {
        return check_overflow(available_bloc) ? available_bloc + 1 : nullptr;
    }

    size_t required_size =
        sizeof(HEADER) + bloc_size + sizeof(long);

    size_t allocation_size = PREALLOC_SIZE;

    // If the request is too large, allocate enough for it.
    if (allocation_size < required_size)
        allocation_size = required_size;

    void* mem = sbrk(allocation_size);

    if (mem == (void*)-1)
    {
        perror("sbrk");
        return nullptr;
    }

    HEADER* new_bloc = (HEADER*)mem;

    new_bloc->ptr_next = nullptr;
    new_bloc->bloc_size = allocation_size - sizeof(HEADER) - sizeof(long);
    new_bloc->magic_number = MAGIC_NUMBER;

    long magic = MAGIC_NUMBER;
    long* footer = (long*)((char*)(new_bloc + 1)+ new_bloc->bloc_size);
    memcpy(footer, &magic, sizeof(long));

    // Insert the newly acquired region into the free list.
    insert_bloc(new_bloc);

    return malloc(bloc_size);
}

void free(void* mem)
{
    HEADER* header = ((HEADER*)mem)-1;
    if (check_overflow(header))
    {
        insert_bloc(header);
    }
}

int main()
{
    char* var1 = malloc(VAR_SIZE_1);

    printf("======ADDRESS  VAR1======\n");
    printf("%p\n",var1);

    printf("======VALUE======\n");
    strcpy(var1,"12345");
    printf("%s\n",var1);

    printf("======HEADER======\n");
    HEADER* header1 = ((HEADER*)var1)-1;
    printf("%p\n",header1->ptr_next);
    printf("%lu\n",header1->bloc_size);
    printf("%lx\n",header1->magic_number);

    printf("======FOOTER======\n");
    printf("%lx\n",*((long*)(var1 + VAR_SIZE_1)));

    char* var2 = malloc(VAR_SIZE_3);

    printf("======ADDRESS  VAR2======\n");
    printf("%p\n",var2);

    printf("======VALUE======\n");
    strcpy(var2,"Bonjour");
    printf("%s\n",var2);

    printf("======HEADER======\n");
    HEADER* header2 = ((HEADER*)var2)-1;
    printf("%p\n",header2->ptr_next);
    printf("%lu\n",header2->bloc_size);
    printf("%lx\n",header2->magic_number);

    printf("======FOOTER======\n");
    printf("%lx\n",*((long*)(var2 + VAR_SIZE_3)));

    printf("======RECYCLE TEST======\n");
    free(var1);
    free(var2);
    print();

    char* var3 = malloc(VAR_SIZE_1);
    printf("======ADDRESS VAR3======\n");
    printf("%p\n",var3);

    print();

    printf("======SPLIT TEST======\n");
    //adresses are 38 bytes apart (24 for HEADER + 6 for DATA + 8 for the FOOTER)

    char* var4 = malloc(VAR_SIZE_1);

    printf("======ADDRESS VAR4======\n");
    printf("%p\n",var4);

    print();

    char* var5 = malloc(VAR_SIZE_2);

    printf("======ADDRESS VAR5======\n");
    printf("%p\n",var5);

    print();

    free(var4);

    print();

    free(var5);

    print();

    return 0;
}
