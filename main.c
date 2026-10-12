#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC_NUMBER 0xBADA110CBADA110CL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next;
    size_t bloc_size;
    long magic_number;
} HEADER;

static HEADER* list = nullptr;

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
        perror("Overflow");
        return false;
    }

    long footer = *(long*)((char*)(header + 1) + size);

    if (footer != MAGIC_NUMBER)
    {
        perror("Overflow");
        return false;
    }
    return true;
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

    if (it > bloc)
    {
        bloc->ptr_next = it;
        list = bloc;
        return;
    }

    while (it && it < bloc)
    {
        prev = it;
        it = it->ptr_next;
    }
    prev->ptr_next = bloc;
    bloc->ptr_next = it;
}

static void* malloc_3is(size_t bloc_size)
{
    size_t total_size = sizeof(HEADER) + bloc_size + sizeof(long);
    HEADER* available_bloc = get_bloc(bloc_size);
    if (!available_bloc)
    {
        void* mem = sbrk(total_size);
        if (mem == (void*)-1)
        {
            perror("sbrk");
            return nullptr;
        }

        HEADER* free_mem = (HEADER*)mem;
        free_mem->ptr_next = nullptr;
        free_mem->bloc_size = bloc_size;
        free_mem->magic_number = MAGIC_NUMBER;

        //+1 to increment by the size of the HEADER and cast the char to increment byte by byte
        long* footer = (long*)((char*)(free_mem + 1) + bloc_size);
        long magic = MAGIC_NUMBER;
        memcpy(footer, &magic, sizeof(long));

        return check_overflow(free_mem) ? free_mem + 1 : nullptr;
    }
    printf("available\n");

    return check_overflow(available_bloc) ? available_bloc + 1 : nullptr;

}

static void free_3is(void* mem)
{
    HEADER* header = ((HEADER*)mem)-1;
    if (check_overflow(header))
    {
        insert_bloc(header);
    }
}

int main()
{
    char* var1 = malloc_3is(6);

    printf("======ADDRESS======\n");
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
    printf("%lx\n",*((long*)(var1 + 6)));

    char* var2 = malloc_3is(8);

    printf("======ADDRESS======\n");
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
    printf("%lx\n",*((long*)(var2 + 8)));

    free_3is(var1);

    char* var3 = malloc_3is(6);

    printf("======ADDRESS======\n");
    printf("%p\n",var3);

    printf("======VALUE======\n");
    strcpy(var3,"12345");
    printf("%s\n",var3);

    printf("======HEADER======\n");
    HEADER* header3 = ((HEADER*)var3)-1;
    printf("%p\n",header3->ptr_next);
    printf("%lu\n",header3->bloc_size);
    printf("%lx\n",header3->magic_number);

    printf("======FOOTER======\n");
    printf("%lx\n",*((long*)(var3 + 6)));
    return 0;
}
