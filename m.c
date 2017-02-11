#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

uint64_t upper_align(const uint64_t size, const uint64_t align) {
  return (size + PAGE_SIZE - 1) & (~(align - 1));
}

void (*orig_free) (void *__ptr , const void *);

typedef struct Node {
  void *head;
  uint64_t size;
} Node;

static void *my_alloc(size_t size, const void *caller) {
  uint64_t alloc_size = upper_align(size + sizeof(Node), PAGE_SIZE) + PAGE_SIZE;

  void *head = memalign(PAGE_SIZE, alloc_size);
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

    __free_hook = orig_free;
    free(node->head);
    __free_hook = my_free;
  }
}

static void my_init(void) {
  __malloc_hook = my_alloc;
  orig_free = __free_hook;
  __free_hook = my_free;
}

void (*__malloc_initialize_hook) (void) = my_init;

int main() {
  FILE *fd = fopen("./m.c", "r");
  fclose(fd);
  void *m = malloc(65536);
  memset(m, 0, 65536);
}
