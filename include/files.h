#ifndef FILES_H
#define FILES_H

#include <stddef.h>      /* para size_t, ya saben */
#include "http.h"        /* para http_status_t, o sea los codigos */

#define FILES_WWW_ROOT "www"   /* la carpeta donde estan las paginas, digamos */

/* Bueno, aqui se guarda el archivo que leimos */
typedef struct {
    http_status_t status;   /* si salio bien es HTTP_OK, y si no pues el error (400, 403, 404, 500) */
    char         *data;     /* todo el contenido del archivo (con malloc), o NULL si no hay */
    size_t        size;     /* lo que pesa en bytes, basicamente */
} file_content;

/* Esta busca el archivo de la uri pero de forma segura, o sea que no te
 * deja salir de www/, y lo lee entero a memoria. */
void files_read_secure(const char *uri, file_content *file);

/* Y esta libera la memoria del archivo, porque si no, este, se queda ahi ocupando. */
void files_free(file_content *file);

#endif
