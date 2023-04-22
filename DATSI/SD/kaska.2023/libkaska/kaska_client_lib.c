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
#define NUM_REQ 3

int client_fd;	  
struct addrinfo *res;

// inicializa el socket y se conecta al servidor
static int init_socket_client() {    
    // socket stream para Internet: TCP
    if ((client_fd=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket");
        return -1;
    }

    char *HOST=getenv("BROKER_HOST");
    char *PORT=getenv("BROKER_PORT");

    // obtiene la dirección TCP remota
    if (getaddrinfo(HOST, PORT, NULL, &res)!=0) {
        perror("error en getaddrinfo");
        close(client_fd);
        return -1;
    }
    // realiza la conexión
    if (connect(client_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("error en connect");
        close(client_fd);
        return -1;
    }
    freeaddrinfo(res);
    return client_fd;
}

// inits socket connection before clients main execution
__attribute__((constructor)) void inicio(void) {
    if (init_socket_client() < 0) {
        _exit(1);
    }
}


// Crea el tema especificado.
// Devuelve 0 si OK y un valor negativo en caso de error.
int create_topic(char *topic) {
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

    if (writev(client_fd, iov, 3) < 0) {
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
int ntopics(void) {
    int op_code = htonl(1);
    struct iovec iov[1];

    // op code
    iov[0].iov_base = &op_code;
    iov[0].iov_len = sizeof(op_code);

    if ((writev(client_fd, iov, 1)) < 0) {
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

// Envía el mensaje al tema especificado; nótese la necesidad
// de indicar el tamaño ya que puede tener un contenido de tipo binario.
// Devuelve el offset si OK y un valor negativo en caso de error.
int send_msg(char *topic, int msg_size, void *msg) {
    return 0;
}
// Devuelve la longitud del mensaje almacenado en ese offset del tema indicado
// y un valor negativo en caso de error.
int msg_length(char *topic, int offset) {
    return 0;
}
// Obtiene el último offset asociado a un tema en el broker, que corresponde
// al del último mensaje enviado más uno y, dado que los mensajes se
// numeran desde 0, coincide con el número de mensajes asociados a ese tema.
// Devuelve ese offset si OK y un valor negativo en caso de error.
int end_offset(char *topic) {
    return 0;
}

// TERCERA FASE: SUBSCRIPCIÓN


// Se suscribe al conjunto de temas recibidos. No permite suscripción
// incremental: hay que especificar todos los temas de una vez.
// Si un tema no existe o está repetido en la lista simplemente se ignora.
// Devuelve el número de temas a los que realmente se ha suscrito
// y un valor negativo solo si ya estaba suscrito a algún tema.
int subscribe(int ntopics, char **topics) {
    return 0;
}

// Se da de baja de todos los temas suscritos.
// Devuelve 0 si OK y un valor negativo si no había suscripciones activas.
int unsubscribe(void) {
    return 0;
}

// Devuelve el offset del cliente para ese tema y un número negativo en
// caso de error.
int position(char *topic) {
    return 0;
}

// Modifica el offset del cliente para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int seek(char *topic, int offset) {
    return 0;
}

// CUARTA FASE: LEER MENSAJES

// Obtiene el siguiente mensaje destinado a este cliente; los dos parámetros
// son de salida.
// Devuelve el tamaño del mensaje (0 si no había mensaje)
// y un número negativo en caso de error.
int poll(char **topic, void **msg) {
    return 0;
}

// QUINTA FASE: COMMIT OFFSETS

// Cliente guarda el offset especificado para ese tema.
// Devuelve 0 si OK y un número negativo en caso de error.
int commit(char *client, char *topic, int offset) {
    return 0;
}

// Cliente obtiene el offset guardado para ese tema.
// Devuelve el offset y un número negativo en caso de error.
int commited(char *client, char *topic) {
    return 0;
}

