#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define ALIGNMENT 16  // Must be power of 2
#define GET_PAD(x) ((ALIGNMENT - 1) - (((x)-1) & (ALIGNMENT - 1)))

#define PADDED_SIZE(x) ((x) + GET_PAD(x))
#define PTR_OFFSET(p, offset) ((void *)((char *)(p) + (offset)))

struct block {
    struct block *next;
    int size;    // bytes
    int in_use;  // bool
};

static struct block* head = NULL;

void print_data() {
    struct block* b = head;

    if (b == NULL) {
        printf("[empty]\n");
        return;
    }

    while (b != NULL) {
        // Uncomment the following line if you want to see the pointer values
        // printf("[%p:%d,%s]", b, b->size, b->in_use? "used": "free");
        printf("[%d,%s]", b->size, b->in_use ? "used" : "free");
        if (b->next != NULL) {
            printf(" -> ");
        }

        b = b->next;
    }

    printf("\n");
}

void coalesc() {
    struct block* current = head;
    while (current != NULL && current->next != NULL) {
        struct block* next = current->next;
        if (current->in_use == 0 && next->in_use == 0) {
            int padded_block_size = PADDED_SIZE(sizeof(struct block));
            current->size += next->size + padded_block_size;
            current->next = next->next;
        } else
            current = next;
    }
}

void myfree(void* p) {
    struct block* node = PTR_OFFSET(p, -PADDED_SIZE(sizeof(struct block)));
    node->in_use = 0;
    coalesc();
}

void split_space(struct block* current, int size) {
    int available_space = current->size;
    int padded_requested_space = PADDED_SIZE(size);
    int padded_block_size = PADDED_SIZE(sizeof(struct block));
    int min_space = padded_requested_space + padded_block_size;
    if (available_space < min_space + 16) return;

    struct block* split_tail = PTR_OFFSET(current, min_space);
    split_tail->size = available_space - min_space;
    split_tail->next = current->next;
    current->size = padded_requested_space;
    current->next = split_tail;
}

void* myalloc(int amount) {
    if (head == NULL) {
        head = mmap(NULL, 1024, PROT_READ|PROT_WRITE,
                    MAP_ANON|MAP_PRIVATE, -1, 0);
        head->next = NULL;
        head->size = 1024 - PADDED_SIZE(sizeof(struct block));
        head->in_use = 0;
    }
    struct block* pointer = head;
    int padded_amount = PADDED_SIZE(amount);
    while (pointer != NULL) {
        if (pointer->in_use == 0 && pointer->size >= padded_amount) {
            split_space(pointer, amount);

            pointer->in_use = 1;
            int padded_block_size = PADDED_SIZE(sizeof(struct block));

            return PTR_OFFSET(pointer, padded_block_size);
        }
        pointer = pointer->next;
    }
    return NULL;
}

int main() {

    void *p, *q, *r, *s;

    p = myalloc(10); print_data();
    q = myalloc(20); print_data();
    r = myalloc(30); print_data();
    s = myalloc(40); print_data();

    myfree(q); print_data();
    myfree(p); print_data();
    myfree(s); print_data();
    myfree(r); print_data();
    return 0;

}