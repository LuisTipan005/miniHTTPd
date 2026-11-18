#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>      /* esto es para size_t, o sea, tamaños */
#include <sys/types.h>   /* y esto para off_t, que tambien es como un tamaño pero de archivos */

/* Bueno, aqui estan los codigos de respuesta que usamos, tipo el 200 que es
 * todo bien, el 404 que no existe y asi. */
typedef enum {
    HTTP_OK                 = 200,
    HTTP_BAD_REQUEST        = 400,
    HTTP_FORBIDDEN          = 403,
    HTTP_NOT_FOUND          = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_ERROR     = 500
} http_status_t;

/* Esto es, digamos, donde guardamos lo que el cliente nos pidio */
typedef struct{
    char method[16];
    char uri[256];
    char version[16];

    // y estos son los encabezados basicos que nos piden, o sea los importantes

    char host[256];
    char connection[64];
    char user_agent[256];

    int keep_alive;

} http_request_t;

/* Esta lee la peticion que llega en texto y la mete en req, basicamente. si todo esta bien devuelve 0 y si no pues -1. */
int http_parse_request(const char *raw_request, http_request_t *req);

/* Y esto que arma los encabezados de la respuesta en buf. Devuelve cuantos bytes escribio, o -1 si no le alcanzo el espacio. */
int http_build_headers(char *buf, size_t buf_sz, http_status_t status,
                       const char *content_type, off_t content_length, int keep_alive);

#endif
