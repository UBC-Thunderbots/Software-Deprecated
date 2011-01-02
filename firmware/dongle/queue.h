#ifndef QUEUE_H
#define QUEUE_H

/**
 * \file
 *
 * \brief Provides convenient and type-safe ways to define linked-list-based queues.
 *
 * To use the definitions in this file, the structure being stored needs to have a field of type pointer to itself called \c next.
 */

/**
 * \brief Evaluates to the name of a queue of some type.
 *
 * \param[in] etype the element type to hold.
 */
#define QUEUE_TYPE(etype) queue_of_ ## etype

/**
 * \brief Defines the type of a queue of elements.
 *
 * \param[in] etype the element type to hold.
 */
#define QUEUE_DEFINE_TYPE(etype) typedef struct { __data etype *queue_head; __data etype *queue_tail; } QUEUE_TYPE(etype)

/**
 * \brief An initializer for an empty queue.
 */
#define QUEUE_INITIALIZER { 0, 0 }

/**
 * \brief Initializes a queue.
 *
 * \param[in] q the queue to initialize.
 */
#define QUEUE_INIT(q) do { (q).queue_head = 0; } while (0)

/**
 * \brief Pushes an element on a queue.
 *
 * \param[in] q the queue to push an element onto.
 *
 * \param[in] elt the element to push.
 */
#define QUEUE_PUSH(q, elt) do { if ((q).queue_head) { (q).queue_tail->next = (elt); } else { (q).queue_head = (elt); } (q).queue_tail = (elt); (elt)->next = 0; } while (0)

/**
 * \brief Returns the next element in a queue.
 *
 * \param[in] q the queue to examine.
 *
 * \return the next element, or null if the \p q is empty.
 */
#define QUEUE_FRONT(q) ((q).queue_head)

/**
 * \brief Removes the first element from a queue.
 *
 * \param[in] q the queue to pop.
 */
#define QUEUE_POP(q) do { if ((q).queue_head) { (q).queue_head = (q).queue_head->next; } } while (0)

/**
 * \brief Puts an element back on the front of a queue.
 *
 * \param[in] q the queue to unpop.
 *
 * \param[in] elt the element to put back.
 */
#define QUEUE_UNPOP(q, elt) do { (elt)->next = (q).queue_head; if (!(q).queue_head) { (q).queue_tail = (elt); } (q).queue_head = (elt); } while (0)

#endif

