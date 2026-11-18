#ifndef MIME_H
#define MIME_H

/* Bueno, esta funcion lo que hace es, ver la extension de `real_path`
 * y decir que tipo de archivo es, tipo text/html o image/png.
 * Y si no la conoce pues devuelve application/octet-stream y ya. */
const char *mime_type(const char *real_path);

#endif
