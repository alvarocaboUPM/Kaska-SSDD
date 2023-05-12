/*
 * Incluya en este fichero todas las implementaciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */
#include "comun.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/uio.h>
#include <string.h>
#include <stdbool.h>

// /**
//  * @brief Message constructor
//  * @return Message* new topic
//  */
// Message* new_msg(int size, char *topic_name)
// {
//     Message *msg = malloc(sizeof(struct Message));
//     msg->topic_name = topic_name;
//     msg->body = malloc(size);
//     msg->size = size;
//     return msg;
// }

// /**
//  * @brief Destroys a Message reference
//  */
// void free_msg(Message *msg)
// {
//     free(msg->body);
//     free(msg);
// }

// // Debug functions
// void print_msg(void *ev)
// {
//     Message *msg = ev;
//     fprintf(stderr, "\nIMPRIMIENDO MENSAJE\nTEMA: %s\nBODY: %p",
//             msg->topic_name,
//             msg->body);
// }