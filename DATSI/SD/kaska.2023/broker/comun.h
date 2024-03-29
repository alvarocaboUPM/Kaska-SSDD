/*
 * Incluya en este fichero todas las definiciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */

#ifndef _COMUN_H
#define _COMUN_H        1


#define STRING_MAX ((1 << 16) - 2)
#define MAX_PATH_LEN 256
#define MAX_FILE_LEN 256

// Message struct
typedef struct Message
{
    void *body;
    int size;
    char *topic_name;

} Message;

#endif // _COMUN_H
