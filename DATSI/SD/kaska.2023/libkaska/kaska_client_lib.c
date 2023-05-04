#include "comun.h"
#include "kaska.h"
#include "map.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdbool.h>
#define NUM_REQ 3

int client_fd=-1;
struct addrinfo *res;

//<TopicName,Offset>
map *subbed_table;

typedef struct offset
{
    int o;
}Offset;

// inicializa el socket y se conecta al servidor
static int init_socket_client()
{
    // socket stream para Internet: TCP
    if ((client_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return -1;
    }

    //printf("Socket iniciado con FD: %d\n", client_fd);
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

/**
 * @brief Frees a subbed_table entry
 * 
 * @param k String
 * @param v Offset
 */
static void free_entry(void *k, void *v) {
    if (v) free(v);
}

/**
 * @brief Checks for offset diff with the server
 * 
 * @param m map
 * @return byte[] with the values
 */
static int map_polling(char* topic, Offset* off){
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
    recv(client_fd, &response, sizeof(response), MSG_WAITALL);
    return response;
}

static void print_subbed_map(){
    map_position*p = map_alloc_position(subbed_table); 
    map_iter * it = map_iter_init(subbed_table,p);
    char* key;
    Offset* o;
    int i=0;

    puts("");
    for (; it && map_iter_has_next(it); map_iter_next(it)) 
    {
        map_iter_value(it, (const void **) &key, (void **) &o);
        printf("%d:después de subscribe: nombre %s offset %d\n", i++, key, o->o);
    }
    p=map_iter_exit(it);
}


// inits socket connection before clients main execution
// __attribute__((constructor)) void inicio(void)
// {
//     if (init_socket_client() < 0)
//     {
//         _exit(1);
//     }
// }

// Crea el tema especificado.
// Devuelve 0 si OK y un valor negativo en caso de error.
int create_topic(char *topic)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
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
// Devuelve cuántos temas existen en el sistema y un valor negativo
// en caso de error.
int ntopics(void)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
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

// SEGUNDA FASE: PRODUCIR/PUBLICAR

/** Envía el mensaje al tema especificado; nótese la necesidad
 de indicar el tamaño ya que puede tener un contenido de tipo binario.
 Devuelve el offset si OK y un valor negativo en caso de error.
 */
int send_msg(char *topic, int msg_size, void *msg)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
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
// Devuelve la longitud del mensaje almacenado en ese offset del tema indicado
// y un valor negativo en caso de error.
int msg_length(char *topic, int offset)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
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
// Obtiene el último offset asociado a un tema en el broker, que corresponde
// al del último mensaje enviado más uno y, dado que los mensajes se
// numeran desde 0, coincide con el número de mensajes asociados a ese tema.
// Devuelve ese offset si OK y un valor negativo en caso de error.
int end_offset(char *topic)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
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
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
    if (subbed_table!=NULL){
        print_subbed_map();
        return -1;
    }

    int subbed_topics = 0;
    subbed_table = map_create(key_string, 0);

    for (int i = 0; i < ntopics; i++)
    {
        int repetido,v;
        Offset* off=(Offset*)malloc(sizeof(Offset));

        char* t_name=strdup(topics[i]);
        map_get(subbed_table, t_name, &repetido);

        //Existe tema y no duplicado
        if (repetido<0 && (v = end_offset(t_name))>= 0){
            off->o=v;
            map_put(subbed_table, t_name, off);
            subbed_topics++;
        }
    }

    //Debug
    print_subbed_map();

    return subbed_topics;
}

// Se da de baja de todos los temas suscritos.
// Devuelve 0 si OK y un valor negativo si no había suscripciones activas.
int unsubscribe(void)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
    if (subbed_table == NULL)
        return -1;
    int size = map_size(subbed_table);
    if (map_destroy(subbed_table, free_entry)<0|| size == 0)
        return -1;
    subbed_table=NULL;
    return 0;
}

// Devuelve el offset del cliente para ese tema y un número negativo en
// caso de error.
int position(char *topic)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
    int res;
    Offset* r = map_get(subbed_table, topic, &res);
    if (res==0)
    {
        res=r->o;
    }

    return res;
    
}

// Modifica el offset del cliente para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int seek(char *topic, int offset)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
    int res;
    Offset* r = map_get(subbed_table, topic, &res);
    if (res==0)
    {
        r->o=offset;
    }

    return res;
    
}

// CUARTA FASE: LEER MENSAJES

// Obtiene el siguiente mensaje destinado a este cliente; los dos parámetros
// son de salida.
// Devuelve el tamaño del mensaje (0 si no había mensaje)
// y un número negativo en caso de error.
int poll(char **topic, void **msg)
{
    if(client_fd==-1){
        printf("Nueva conexión en cliente %d\n", init_socket_client());
    }
    map_position* p = map_alloc_position(subbed_table);
    map_iter * it = map_iter_init(subbed_table,p);
    char* key;
    Offset* o;
    bool found;

    for (found=false; found!=true && it && map_iter_has_next(it); map_iter_next(it)) 
    {
        map_iter_value(it, (const void **) &key, (void **) &o);
        if(msg_length(key,o->o)>0)
        ;
        
    }
    return 0;
    //return map_iter_exit(it);
}

// QUINTA FASE: COMMIT OFFSETS

// Cliente guarda el offset especificado para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int commit(char *client, char *topic, int offset)
{
    return 0;
}

// Cliente obtiene el offset guardado para ese tema.
// Devuelve el offset y un número negativo en caso de error.
int commited(char *client, char *topic)
{
    return 0;
}