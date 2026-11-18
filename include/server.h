#ifndef SERVER_H
#define SERVER_H

#define SERVER_PORT          2017   /* este es el puerto donde, o sea, escucha el server */
#define SERVER_MAX_EVENTS    64     /* cuantos eventos agarra epoll_wait de una sola vez, tipo maximo */
#define SERVER_MAX_CLIENTS   1024   /* el tamaño de la tabla last_active, el indice es el fd */
#define SERVER_IDLE_TIMEOUT  10     /* los segundos que aguanta sin hacer nada antes de cerrar */

/* Bueno, esto pone el socket sd en no bloqueante, o sea que no se queda
 * esperando. Si sale bien devuelve 0 y si falla pues -1. */
int set_nonblocking(int sd);

/*
 * Esta, este, crea el socket de escucha en port. Basicamente hace el
 * SO_REUSEADDR, el bind, lo pone no bloqueante y luego el listen.
 * Devuelve el socket (sd), o -1 si algo sale mal, asi de simple.
 */
int create_listen_socket(int port);

#endif /* SERVER_H */
