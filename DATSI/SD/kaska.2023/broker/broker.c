/*========================================================
 * 			   BROKER.c by Álvaro Cabo
 *			  	    Versión 0.1
 *			  	     20/04/2023
 * ========================================================
 */

//=========INCLUDES=========

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include "comun.h"
#include "map.h"
#include "queue.h"

#define STRING_MAX ((1 << 16) - 2)
//========GLOBAL VARIABLES & STRUCTS==========

int server_fd;
struct sockaddr_in dir;
int opcion;

// Data structures

map *table_clients;
map *table_topics;

// information given to the working thread
typedef struct thread_info
{
    int socket;
} thread_info;

// Topic struct
typedef struct Topic
{
    char *name; // key
    queue *messages;
    map *subscribers;
} Topic;

// Message struct
typedef struct Message
{
    void *body;
    int size;
    char *topic_name;
    int subbed_clients;
} Message;

// Client struct
typedef struct Sub
{
    char *SID; // key
    int offset;
    map *sub_topics;
} Sub;

//========STATIC FUNCTIONS==========

/**
 * @brief inicializa el socket y lo prepara para aceptar conexiones
 *
 * @param port
 * @return int
 */
static int init_socket_server(const char *port)
{
    opcion = 1;
    // socket stream para Internet: TCP
    if ((server_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return -1;
    }
    // Para reutilizar puerto inmediatamente si se rearranca el servidor
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0)
    {
        perror("error en setsockopt");
        return -1;
    }
    // asocia el socket al puerto especificado
    dir.sin_addr.s_addr = INADDR_ANY;
    dir.sin_port = htons(atoi(port));
    dir.sin_family = PF_INET;
    if (bind(server_fd, (struct sockaddr *)&dir, sizeof(dir)) < 0)
    {
        perror("error en bind");
        close(server_fd);
        return -1;
    }
    // establece el nº máx. de conexiones pendientes de aceptar
    if (listen(server_fd, 5) < 0)
    {
        perror("error en listen");
        close(server_fd);
        return -1;
    }
    return server_fd;
}

/*
    STRUCT IMPLEMENTATIONS
*/

/**
 * @brief Topic constructor
 * @return Topic* new topic
 */
Topic *new_topic(char *name)
{
    Topic *newTopic = malloc(sizeof(struct Topic));
    newTopic->name = strdup(name);
    newTopic->messages = queue_create(0);
    newTopic->subscribers = map_create(key_string, 0);
    return newTopic;
}

/**
 * @brief Message constructor
 * @return Message* new topic
 */
Message *new_msg(int size, char *topic_name)
{
    Message *msg = malloc(sizeof(struct Message));
    msg->topic_name = strdup(topic_name);
    msg->subbed_clients = 0;
    msg->body = malloc(size);
    msg->size = size;
    return msg;
}

/**
 * @brief Destroys a Message pointer
 */
void free_msg(Message *msg)
{
    free(msg->body);
    free(msg);
}

/*
        SERVER FUNCTIONS
*/

void exit_handler()
{
    close(server_fd);
    puts("\n\n\tCLOSING BROKER\n\n");
    exit(0);
}

/**
 * @brief Get the topic name object
 *
 * @param client_fd
 * @return char* topic_name
 */
char *get_topic_name(int client_fd)
{
    int raw_size, size;
    char *topic_name;

    // 1. Get string size
    recv(client_fd, &raw_size, sizeof(int), MSG_WAITALL);
    size = ntohl(raw_size);

    if (size > STRING_MAX)
        size = STRING_MAX;

    topic_name = malloc(size + 1);
    // 2. Get topic name
    recv(client_fd, topic_name, size, MSG_WAITALL);
    topic_name[size] = '\0';
    return topic_name;
}

/**
 * @brief Server function
 * @include recv
 * @param arg thread_info struct
 */
void *service(void *arg)
{
    int code, response;
    thread_info *thinf = arg;

    while (recv(thinf->socket, &code, sizeof(int), MSG_WAITALL) == sizeof(int))
    {
        // Parsing the request
        code = ntohl(code);
        printf("Recibido operation code: %d\n", code);

        switch (code)
        {
        // new_topic
        case 0:
            char *topic_name = get_topic_name(thinf->socket);
            Topic* topic = new_topic(topic_name);
            response=map_put(table_topics, topic_name, topic);
            break;
        // n_topics
        case 1:
            response= htonl(map_size(table_topics));
            break;
        default:
            fprintf(stderr, "Operation not allowed with code %d\n", code);
            break;
        }

        // envía un code como respuesta
        send(thinf->socket, &response, sizeof(response), 0);
    }
    close(thinf->socket);
    return NULL;
}

/**
 * @brief Main thread function
 *
 * @param argc
 * @param argv
 * @return 0 on success | -1 for error
 */
int main(int argc, char *argv[])
{
    int s_conec;
    unsigned int tam_dir;
    struct sockaddr_in dir_cliente;

    if (signal(SIGINT, &exit_handler) == SIG_ERR)
    {
        exit(EXIT_FAILURE);
    }
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return -1;
    }
    // inicializa el socket y lo prepara para aceptar conexiones
    if ((server_fd = init_socket_server(argv[1])) < 0)
        return -1;

    // prepara atributos adecuados para crear thread "detached"
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); // evita pthread_join
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);

    // inicializa estructuras de datos

    if ((table_topics = map_create(key_string, 0)) == NULL ||
        (table_clients = map_create(key_string, 0)) == NULL)
    {
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        tam_dir = sizeof(dir_cliente);
        // acepta la conexión
        if ((s_conec = accept(server_fd, (struct sockaddr *)&dir_cliente, &tam_dir)) < 0)
        {
            perror("error en accept");
            close(server_fd);
            return -1;
        }
        // crea el thread de service
        thread_info *thinf = malloc(sizeof(thread_info));
        thinf->socket = s_conec;
        pthread_create(&thid, &atrib_th, service, thinf);
    }
    close(server_fd); // cierra el socket general
    return 0;
}
