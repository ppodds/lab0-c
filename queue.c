#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    // allocate space for queue node
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;
    // initialize list member
    INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    // check if list is NULL
    if (l == NULL)
        return;
    struct list_head *li, *tmp;
    list_for_each_safe (li, tmp, l) {
        element_t *elem = list_entry(li, element_t, list);
        list_del(li);
        q_release_element(elem);
    }
    // free queue head
    free(l);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    // check if queue is NULL
    if (!head)
        return false;
    element_t *node = malloc(sizeof(element_t));
    if (!node)
        return false;
    INIT_LIST_HEAD(&node->list);
    // insert the node to the beginning of the queue
    list_add(&node->list, head);
    node->value = strdup(s);
    if (!node->value) {
        list_del(&node->list);
        free(node);
        return false;
    }
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    // check if queue is NULL
    if (!head)
        return false;
    element_t *node = malloc(sizeof(element_t));
    if (!node)
        return false;
    INIT_LIST_HEAD(&node->list);
    // insert the node to the end of the queue
    list_add_tail(&node->list, head);
    node->value = strdup(s);
    if (!node->value) {
        list_del(&node->list);
        free(node);
        return false;
    }
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    // if the queue is NULL or empty, return NULL
    if (!head || list_empty(head))
        return NULL;

    element_t *node = list_first_entry(head, element_t, list);
    list_del(&node->list);
    // check if sp is non-NULL
    if (sp) {
        strncpy(sp, node->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return node;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    // if the queue is NULL or empty, return NULL
    if (!head || list_empty(head))
        return NULL;

    element_t *node = list_last_entry(head, element_t, list);
    list_del(&node->list);
    // check if sp is non-NULL
    if (sp) {
        strncpy(sp, node->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return node;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;

    struct list_head *slow;
    struct list_head *fast = head->next;

    list_for_each (slow, head) {
        // find the middle node
        if (fast == head || fast->next == head)
            break;
        fast = fast->next->next;
    }
    list_del(slow);
    q_release_element(list_entry(slow, element_t, list));
    return true;
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    if (!head)
        return false;

    struct list_head *li, *safe;
    bool dup = false;
    for (li = (head)->next, safe = li->next; li != (head) && safe != (head);
         li = safe, safe = li->next) {
        element_t *cur = list_entry(li, element_t, list);
        element_t *next = list_entry(li->next, element_t, list);
        if (strcmp(cur->value, next->value) == 0) {
            list_del(li);
            q_release_element(cur);
            dup = true;
        } else if (dup) {
            list_del(li);
            q_release_element(cur);
            dup = false;
        }
    }
    return true;
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    struct list_head *cur = head->next;
    while (cur != head && cur->next != head) {
        list_move(cur, cur->next);
        cur = cur->next;
    }
    // https://leetcode.com/problems/swap-nodes-in-pairs/
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    // if the queue is NULL or its size <= 1, return
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    struct list_head *cur = head->prev;
    head->prev = head->next;
    head->next = cur;

    while (cur != head) {
        struct list_head *tmp = cur->prev;
        cur->prev = cur->next;
        cur->next = tmp;
        cur = tmp;
    }
}

static int cmp(struct list_head *a, struct list_head *b)
{
    const int result = strcmp(list_entry(a, element_t, list)->value,
                              list_entry(b, element_t, list)->value);
    return result > 0 ? 1 : 0;
}

/*
 * Returns a list organized in an intermediate format suited
 * to chaining of merge() calls: null-terminated, no reserved or
 * sentinel head node, "prev" links not maintained.
 */
static struct list_head *merge(struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

/*
 * Combine final list merge with restoration of standard doubly-linked
 * list structure.  This approach duplicates code from merge(), but
 * runs faster than the tidier alternatives of either a separate final
 * prev-link restoration pass, or maintaining the prev links
 * throughout.
 */
static void merge_final(struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head)
        return;

    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0; /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    /*
     * Data structure invariants:
     * - All lists are singly linked and null-terminated; prev
     *   pointers are not maintained.
     * - pending is a prev-linked "list of lists" of sorted
     *   sublists awaiting further merging.
     * - Each of the sorted sublists is power-of-two in size.
     * - Sublists are sorted by size and age, smallest & newest at front.
     * - There are zero to two sublists of each size.
     * - A pair of pending sublists are merged as soon as the number
     *   of following pending elements equals their size (i.e.
     *   each time count reaches an odd multiple of that size).
     *   That ensures each later final merge will be at worst 2:1.
     * - Each round consists of:
     *   - Merging the two sublists selected by the highest bit
     *     which flips when count is incremented, and
     *   - Adding an element from the input as a size-1 sublist.
     */
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (!!(bits) == 1) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(head, pending, list);
}
