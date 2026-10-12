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

static bool is_available(size_t size)
{
    HEADER* it = list;
    while (it)
    {
        if (it->bloc_size > size)
            return true;
        it = it->ptr_next;
    }
    return false;
}

void* malloc_3is(size_t bloc_size)
{
    size_t total_size = sizeof(HEADER) + bloc_size + sizeof(long);
    if (!is_available(total_size))
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

        return free_mem + 1;
    }
    printf("available");
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

    char* var2 = malloc_3is(6);

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
    printf("%lx\n",*((long*)(var2 + 6)));

    return 0;
}
