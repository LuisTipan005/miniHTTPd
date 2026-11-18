#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "http.h"

/* Bueno, esta revisa si hay un encabezado con ese nombre, o sea, va linea
 * por linea viendo si alguna empieza con name */
static int header_exists(const char *raw_request, const char *name)
{
    const char *p = raw_request;
    unsigned long name_len = strlen(name);

    while ((p = strchr(p, '\n')) != NULL) {
        p++;

        /* saltamos los espacios y esas cosas del inicio, tipo basura */
        while (*p == ' ' || *p == '\t' || *p == '\r')
            p++;

        if (strncasecmp(p, name, name_len) == 0)
            return 1;
    }

    return 0;
}

/* Esta ve si el cliente quiere cerrar la conexion, o sea si dice
 * "Connection: close" cerramos; si no, en HTTP/1.1 se mantiene abierta */
static int request_wants_close(const char *raw_request)
{
    if (header_exists(raw_request, "Connection:")) {
        if (strcasestr(raw_request, "Connection: close") != NULL)
            return 1;
    }

    return 0;
}

int http_parse_request(const char *raw_request, http_request_t *req)
{
    http_request_t r;

    /* primero limpiamos todo, o sea lo dejamos en cero */
    memset(&r, 0, sizeof(r));

    /* si no tiene el \r\n\r\n del final es que, este, esta incompleta */
    if (strstr(raw_request, "\r\n\r\n") == NULL)
        return -1;

    /* aqui sacamos el metodo, la uri y la version, y ojo que con limite
       para que no se desborde, ¿si? */
    if (sscanf(raw_request, "%15s %255s %15s", r.method, r.uri, r.version) != 3)
        return -1;

    /* solo aceptamos HTTP/1.1, lo demas pues no */
    if (strcmp(r.version, "HTTP/1.1") != 0)
        return -1;

    /* en HTTP/1.1 el Host es obligatorio, o sea si no esta es error */
    if (!header_exists(raw_request, "Host:"))
        return -1;

    r.keep_alive = !request_wants_close(raw_request);

    *req = r;
    return 0;
}

/* Bueno, esta nomas convierte el codigo en el texto, tipo 404 -> "Not Found" */
static const char *http_status_text(http_status_t status)
{
    if (status == HTTP_OK)
        return "OK";
    else if (status == HTTP_BAD_REQUEST)
        return "Bad Request";
    else if (status == HTTP_FORBIDDEN)
        return "Forbidden";
    else if (status == HTTP_NOT_FOUND)
        return "Not Found";
    else if (status == HTTP_METHOD_NOT_ALLOWED)
        return "Method Not Allowed";
    else
        return "Internal Server Error";
}

int http_build_headers(char *buf, size_t buf_sz, http_status_t status,
                       const char *content_type, off_t content_length, int keep_alive)
{
    int written;

    /* armamos los encabezados con snprintf, que es como el seguro porque
       no se pasa del tamaño del buffer, basicamente */
    written = snprintf(buf, buf_sz,
                       "HTTP/1.1 %d %s\r\n"
                       "Content-Type: %s\r\n"
                       "Content-Length: %ld\r\n"
                       "Connection: %s\r\n"
                       "\r\n",
                       (int)status, http_status_text(status), content_type,
                       (long)content_length,
                       keep_alive ? "keep-alive" : "close");

    /* si no alcanzo el espacio pues, este, devolvemos error */
    if (written < 0 || (size_t)written >= buf_sz)
        return -1;

    return written;
}