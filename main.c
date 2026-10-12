#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAGIC_NUMBER 0xBADA110CBADA110CL

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0xBADA110CBADA110CL */
} HEADER;

static HEADER* list = NULL;
static size_t max_size = 0;

static HEADER* available(size_t size)
{
    HEADER* it = list;
    while (it)
    {
        if (it->bloc_size == size)
            return it;
        it = it->ptr_next;
    }
    return nullptr;
}

bool check_overflow(void* mem)
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

static void insertBloc(void* mem)
{
    HEADER* bloc = (HEADER*)mem;

    if (list == NULL)
    {
        bloc->ptr_next = NULL;
        list = bloc;
        return;
    }

    HEADER* it = list;
    HEADER* prev = NULL;
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

void* malloc_3is(size_t bloc_size)
{
    size_t total_size = sizeof(HEADER) + bloc_size + sizeof(long);
    HEADER* available_bloc = available(bloc_size);
    if (!available_bloc)
    {
        void* mem = sbrk(total_size);
        if (mem == (void*)-1)
        {
            perror("sbrk");
            return nullptr;
        }

        HEADER* free_mem = (HEADER*)mem;
        free_mem->ptr_next = NULL;
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

void free_3is(void* mem)
{
    HEADER* header = ((HEADER*)mem)-1;
    if (check_overflow(header))
    {
        insertBloc(header);
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
    printf("%d\n",header1->bloc_size);
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
    printf("%d\n",header2->bloc_size);
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
    printf("%d\n",header3->bloc_size);
    printf("%lx\n",header3->magic_number);

    printf("======FOOTER======\n");
    printf("%lx\n",*((long*)(var3 + 6)));
    return 0;
}
