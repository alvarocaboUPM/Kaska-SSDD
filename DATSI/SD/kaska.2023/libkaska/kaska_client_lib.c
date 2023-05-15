#include "comun.h"
#include "kaska.h"
#include "map.h"
#include <sys/uio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

int client_fd = -1;

//<TopicName,Offset>
map *subbed_table = NULL;
map_position *p = NULL;

typedef struct offset
{
    int o;
} Offset;

// inicializa el socket y se conecta al servidor
static int init_socket_client()
{
    struct addrinfo *res;
    // socket stream para Internet: TCP
    if ((client_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return -1;
    }

    char *HOST = getenv("BROKER_HOST");
    char *PORT = getenv("BROKER_PORT");

    // obtiene la dirección TCP remota
    if (getaddrinfo(HOST, PORT, NULL, &res) != 0)
    {
        perror("error en getaddrinfo");
        close(client_fd);
        return -1;
    }
    // realiza la conexión
    if (connect(client_fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("error en connect");
        close(client_fd);
        return -1;
    }
    freeaddrinfo(res);
    return client_fd;
}

static int crear_conexion()
{
    if (client_fd == -1 && (client_fd = init_socket_client()) < 0)
    {
        perror("Error initializing socket client");
        return -1;
    }
    return 0;
}

/**
 * @brief Frees a subbed_table entry
 *
 * @param k String
 * @param v Offset
 */
static void free_entry(void *k, void *v)
{
    if (v)
        free(v);
}

static void print_subbed_map()
{
    map_position *p_aux = map_alloc_position(subbed_table);
    map_iter *it = map_iter_init(subbed_table, p_aux);
    char *key;
    Offset *o;
    int i = 0;

    puts("");
    for (; it && map_iter_has_next(it); map_iter_next(it))
    {
        map_iter_value(it, (const void **)&key, (void **)&o);
        printf("%d:después de subscribe: nombre %s offset %d\n", i++, key, o->o);
    }
    map_iter_exit(it);
    map_free_position(p_aux);
}

/**
 * @brief Message constructor
 * @return Message* new topic
 */
Message *new_msg(int size, char *topic_name)
{
    Message *msg = malloc(sizeof(struct Message));
    msg->topic_name = topic_name;
    msg->body = malloc(size);
    msg->size = size;
    return msg;
}

/**
 * @brief Destroys a Message reference
 */
void free_msg(Message *msg)
{
    free(msg->body);
    free(msg);
}

// Debug functions
void print_msg(void *ev)
{
    Message *msg = ev;
    fprintf(stderr, "\nIMPRIMIENDO MENSAJE\nTEMA: %s\nSIZE: %d\nBODY: %s\n",
            msg->topic_name,
            msg->size,
            (char *)msg->body);
}

/**
 * @brief Checks for message content
 * @param off queue offset in the topic where trying to find a message
 *
 * @param m map
 * @return byte[] with the values
 */
static int map_polling(char *topic, Offset *off)
{
    int op_code = htonl(5);
    struct iovec iov[5];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;
    // msg size
    int msg_size_nl = htonl(off->o);
    iov[3].iov_base = &msg_size_nl;
    iov[3].iov_len = sizeof(msg_size_nl);

    if (writev(client_fd, iov, 4) < 0)
    {
        perror("error polling a message");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    int response;
    recv(client_fd, &response, sizeof(int), MSG_WAITALL);
    return response;
}

int create_topic(char *topic)
{
    if (crear_conexion() < 0)
        return -1;
    int op_code = htonl(0);
    struct iovec iov[3];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;

    if (writev(client_fd, iov, 3) < 0)
    {
        perror("error creating topic");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

int ntopics(void)
{
    if (crear_conexion() < 0)
        return -1;
    int op_code = htonl(1);
    struct iovec iov[1];

    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);

    if ((writev(client_fd, iov, 1)) < 0)
    {
        perror("error getting number of topics");
        close(client_fd);
        return -1;
    }
    uint32_t tmp, response;
    recv(client_fd, &tmp, sizeof(tmp), MSG_WAITALL);
    response = ntohl(tmp);
    return response;
}

int send_msg(char *topic, int msg_size, void *msg)
{
    if (crear_conexion() < 0)
        return -1;
    int op_code = htonl(2);
    struct iovec iov[5];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;
    // msg size
    int msg_size_nl = htonl(msg_size);
    iov[3].iov_base = &msg_size_nl;
    iov[3].iov_len = sizeof(msg_size_nl);
    // msg body
    iov[4].iov_base = msg;
    iov[4].iov_len = msg_size;

    if (writev(client_fd, iov, 5) < 0)
    {
        perror("error publishing a message");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

int msg_length(char *topic, int offset)
{
    if (crear_conexion() < 0)
        return -1;
    int op_code = htonl(3);
    struct iovec iov[4];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;
    // offset
    int offset_nl = htonl(offset);
    iov[3].iov_base = &offset_nl;
    iov[3].iov_len = sizeof(offset_nl);

    if (writev(client_fd, iov, 4) < 0)
    {
        perror("error getting msg length");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

int end_offset(char *topic)
{
    if (crear_conexion() < 0)
        return -1;
    int op_code = htonl(4);
    struct iovec iov[3];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;

    if (writev(client_fd, iov, 3) < 0)
    {
        perror("error getting num of msgs");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

// TERCERA FASE: SUBSCRIPCIÓN

/**
 * @brief Se suscribe al conjunto de temas recibidos.
 * No permite suscripción incremental: hay que especificar todos los temas de una vez.
 * Si un tema no existe o está repetido en la lista simplemente se ignora.
 *
 * @param ntopics
 * @param topics
 * @return número de temas a los que realmente se ha suscrito | un valor negativo solo si ya estaba suscrito a algún tema.
 */
int subscribe(int ntopics, char **topics)
{
    if (crear_conexion() < 0)
        return -1;
    if (subbed_table != NULL)
    {
        print_subbed_map();
        return -1;
    }

    int subbed_topics = 0;
    if (ntopics < 1)
        return subbed_topics;

    subbed_table = map_create(key_string, 0);

    for (int i = 0; i < ntopics; i++)
    {
        int repetido, v;
        Offset *off = (Offset *)malloc(sizeof(Offset));

        char *t_name = strdup(topics[i]);
        map_get(subbed_table, t_name, &repetido);

        // Existe tema y no duplicado
        if (repetido < 0 && (v = end_offset(t_name)) >= 0)
        {
            off->o = v;
            map_put(subbed_table, t_name, off);
            subbed_topics++;
        }
    }

    // Debug
    print_subbed_map();

    return subbed_topics;
}

int unsubscribe(void)
{
    if (crear_conexion() < 0)
        return -1;
    if (subbed_table == NULL || map_size(subbed_table) == 0 || map_destroy(subbed_table, free_entry) < 0)
        return -1;
    subbed_table = NULL;
    return 0;
}

int position(char *topic)
{
    if (crear_conexion() < 0)
        return -1;
    int res;
    Offset *r = map_get(subbed_table, topic, &res);
    if (res == 0)
    {
        res = r->o;
    }

    return res;
}

int seek(char *topic, int offset)
{
    if (crear_conexion() < 0)
        return -1;
    int res;
    Offset *r = map_get(subbed_table, topic, &res);
    if (res == 0)
    {
        r->o = offset;
    }

    return res;
}

int poll(char **topic, void **msg)
{
    if (crear_conexion() < 0)
        return -1;

    if (subbed_table == NULL)
        return 0;

    if (p == NULL)
        p = map_alloc_position(subbed_table);

    map_iter *it;
    char *key;
    Offset *o;
    int res;
    Message *m;
    char *tmp_topic = NULL;

    if ((it = map_iter_init(subbed_table, p)) == NULL)
    {
        //tries to allocated the position after unsub
        p = map_alloc_position(subbed_table);
        if ((it = map_iter_init(subbed_table, p)) == NULL)
        {
            fprintf(stderr, "Invalid position for init\n");
            return -1;
        }
    }

    for (res = 0; res <= 0 && it && map_iter_has_next(it); map_iter_next(it))
    {
        if (map_iter_value(it, (const void **)&key, (void **)&o) < 0)
        {
            perror("Error getting key-value");
        }

        if ((res = map_polling(key, o)) >= 0)
        {
            tmp_topic = strdup(key);
            m = new_msg(res, tmp_topic);
            recv(client_fd, m->body, res, MSG_WAITALL);
            map_iter_next(it);
            p = map_iter_exit(it);
            *topic = tmp_topic;
            *msg = m->body;
            o->o++;
            return res;
        }
    }

    free(tmp_topic);
    return 0;
}

// QUINTA FASE: COMMIT OFFSETS

// Cliente guarda el offset especificado para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int commit(char *client, char *topic, int offset)
{
    if (crear_conexion() < 0)
        return -1;

    int op_code = htonl(6);
    struct iovec iov[6];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;
    // size of UID
    int client_len = strlen(client);
    int arg_size_2 = htonl(client_len);
    iov[3].iov_base = &arg_size_2;
    iov[3].iov_len = sizeof(arg_size_2);
    // UID
    iov[4].iov_base = client;
    iov[4].iov_len = client_len;
    // offset
    int offset_nl = htonl(offset);
    iov[5].iov_base = &offset_nl;
    iov[5].iov_len = sizeof(offset_nl);

    if (writev(client_fd, iov, 6) < 0)
    {
        perror("error getting msg length");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

// Cliente obtiene el offset guardado para ese tema.
// Devuelve el offset y un número negativo en caso de error.
int commited(char *client, char *topic)
{
    if (crear_conexion() < 0)
        return -1;

    if (crear_conexion() < 0)
        return -1;

    int op_code = htonl(7);
    struct iovec iov[5];
    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);
    // size of topic name
    int topic_len = strlen(topic);
    int arg_size = htonl(topic_len);
    iov[1].iov_base = &arg_size;
    iov[1].iov_len = sizeof(arg_size);
    // topic name
    iov[2].iov_base = topic;
    iov[2].iov_len = topic_len;
    // size of UID
    int client_len = strlen(client);
    int arg_size_2 = htonl(client_len);
    iov[3].iov_base = &arg_size_2;
    iov[3].iov_len = sizeof(arg_size_2);
    // UID
    iov[4].iov_base = client;
    iov[4].iov_len = client_len;

    if (writev(client_fd, iov, 5) < 0)
    {
        perror("error getting msg length");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    int response;
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}
