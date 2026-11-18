#include "mime.h"

#include <string.h>
#include <strings.h>

/* Bueno, esta es como una tablita que dice que extension va con que tipo */
static const struct {
    const char *ext;
    const char *type;
} table[] = {
    { ".html", "text/html" },
    { ".css",  "text/css" },
    { ".js",   "application/javascript" },
    { ".png",  "image/png" },
    { ".jpg",  "image/jpeg" },
};

const char *mime_type(const char *real_path)
{
    /* O sea, buscamos el ultimo punto, pero solo en el nombre del archivo,
       no en las carpetas, ¿si me explico? Por eso tambien buscamos la ultima barra. */
    const char *slash = strrchr(real_path, '/');
    const char *dot = strrchr(real_path, '.');
    if (dot != NULL && (slash == NULL || dot > slash)) {
        for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++)
            if (strcasecmp(dot, table[i].ext) == 0)
                return table[i].type;
    }
    /* y si no la encontro pues devolvemos el generico y listo */
    return "application/octet-stream";
}
