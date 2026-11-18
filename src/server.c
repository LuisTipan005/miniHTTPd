#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"

/* Esta funcion ayuda a que el socket no se quede trabado esperando */
int no_bloqueante(int sd)
{
    int flags = fcntl(sd, F_GETFL, 0);

    if (flags < 0)
        return -1;

    return fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}

/* Aqui creamos el socket de escucha y lo dejamos listo para epoll, o sea, preparadito */
int create_listen_socket(int port)
{
    int sd;
    int opt = 1;
    struct sockaddr_in addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket");
        return -1;
    }

    /* Esto del SO_REUSEADDR es, digamos, para poder volver a usar el puerto
       rapido si reiniciamos el server */
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Aqui le ponemos la IP y el puerto, osea el bind */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sd);
        return -1;
    }

    /* Lo ponemos no bloqueante porque  epoll lo necesita asi */
    if (no_bloqueante(sd) < 0) {
        perror("fcntl");
        close(sd);
        return -1;
    }

    /* Y ya, lo ponemos a escuchar, basicamente con listen */
    if (listen(sd, SOMAXCONN) < 0) {
        perror("listen");
        close(sd);
        return -1;
    }

    return sd;
}
