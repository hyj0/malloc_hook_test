#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

uint64_t upper_align(const uint64_t size, const uint64_t align) {
    return (size + PAGE_SIZE - 1) & (~(align - 1));
}

void (*orig_free) (void *__ptr , const void *);
void *(*orig_memalign)(size_t boundary, size_t size);

typedef struct Node {
    void *head;
    uint64_t size;
} Node;

static void *my_alloc(size_t size, const void *caller) {
    uint64_t alloc_size = upper_align(size + sizeof(Node), PAGE_SIZE) + PAGE_SIZE;

    void *head = orig_memalign(PAGE_SIZE, alloc_size);
    assert(NULL != head);

    void *tail = head + alloc_size - PAGE_SIZE;
    void *ptr = tail - size;
    Node *node = (Node*)(ptr - sizeof(Node));

    node->head = head;
    node->size = size;

    mprotect(tail, PAGE_SIZE, PROT_NONE);

    return ptr;
}

static void my_free(void *ptr, const void *caller) {
    if (NULL != ptr) {
        Node *node = (Node*)(ptr - sizeof(Node));
        void *tail = ptr + node->size;
        mprotect(tail, PAGE_SIZE, PROT_WRITE | PROT_READ);

#if 0   //todo:不明白为什么这里 orig_free为NULL
        orig_free(node->head, NULL);
#else
        //todo:理论上这样写有并发问题
        __free_hook = orig_free;
        free(node->head);
        __free_hook = my_free;
#endif
    }
}

static void *my_memalign(size_t boundary, size_t size) {
    //todo:这里不正确, 需要时再改
    return malloc(size);
}

static void *my_realloc(void * oldBuffer ,size_t newSize, void * caller) {
    void *newBuffer = my_alloc(newSize, NULL);
    if (oldBuffer) {
        Node *node = (Node*)(oldBuffer - sizeof(Node));
        size_t size = node->size;
        if (newSize < size) {
            size = newSize;
        }
        if (size > 0) {
            memcpy(newBuffer, oldBuffer, size);
        }
        my_free(oldBuffer, NULL);
    }
    return newBuffer;
}

static void my_init(void) {
    __malloc_hook = my_alloc;
    orig_free = __free_hook;
    __free_hook = my_free;

    orig_memalign = __memalign_hook;
    __memalign_hook = my_memalign;

    __realloc_hook = my_realloc;
}

void (*__malloc_initialize_hook) (void) = my_init;
