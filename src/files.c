#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include "files.h"

/* Bueno, esta arma la ruta juntando la carpeta www con lo que pidieron y si piden "/" le damos el index.html */
static int build_candidate_path(char *dst, unsigned long dst_size,
                                const char *root_real,
                                const char *url_path)
{
    unsigned long root_len = strlen(root_real);
    unsigned long path_len = strlen(url_path);
    const char *suffix;

    if (strcmp(url_path, "/") == 0)
        suffix = "/index.html";
    else
        suffix = url_path;

    path_len = strlen(suffix);

    /* si no cabe en el buffer, este, mejor ni lo intentamos */
    if (root_len + path_len + 1 > dst_size)
        return -1;

    memcpy(dst, root_real, root_len);
    memcpy(dst + root_len, suffix, path_len);
    dst[root_len + path_len] = '\0';

    return 0;
}

void files_read_secure(const char *uri, file_content *file)
{
    char root_real[PATH_MAX];
    char candidate[PATH_MAX];
    char candidate_real[PATH_MAX];
    struct stat st;
    unsigned long root_len;
    unsigned long total = 0;
    ssize_t n = 0;
    int fd;
    file_content f = {HTTP_INTERNAL_ERROR, NULL, 0};

    // Primero vemos que la uri este bien, o sea que empiece con /
    if (uri == NULL || uri[0] != '/') {
        f.status = HTTP_BAD_REQUEST;
        *file = f;
        return;
    }

    // si tiene ".." es que quiere salirse de la carpeta, tipo hacker, asi que no
    if (strstr(uri, "..") != NULL) {
        f.status = HTTP_FORBIDDEN;
        *file = f;
        return;
    }

    // sacamos la ruta real de la carpeta www, y si falla pues es un 500
    if (realpath(FILES_WWW_ROOT, root_real) == NULL) {
        f.status = HTTP_INTERNAL_ERROR;
        *file = f;
        return;
    }

    if (build_candidate_path(candidate, sizeof(candidate), root_real, uri) < 0) {
        f.status = HTTP_BAD_REQUEST;
        *file = f;
        return;
    }

    // si realpath no lo encuentra es que, este, el archivo no existe
    if (realpath(candidate, candidate_real) == NULL) {
        f.status = HTTP_NOT_FOUND;
        *file = f;
        return;
    }

    // Aqui evitamos el directory traversal, o sea que la ruta tiene que
    // empezar si o si con la carpeta www
    root_len = strlen(root_real);

    if (strncmp(candidate_real, root_real, root_len) != 0) {
        f.status = HTTP_FORBIDDEN;
        *file = f;
        return;
    }

    // y despues de la carpeta tiene que venir una / o el final, porque si no
    // "wwwotra" tambien pasaria, ¿si me entienden?
    if (candidate_real[root_len] != '/' && candidate_real[root_len] != '\0') {
        f.status = HTTP_FORBIDDEN;
        *file = f;
        return;
    }

    if (stat(candidate_real, &st) < 0) {
        f.status = HTTP_NOT_FOUND;
        *file = f;
        return;
    }

    // solo archivos normales, nada de carpetas ni cosas raras
    if (!S_ISREG(st.st_mode)) {
        f.status = HTTP_FORBIDDEN;
        *file = f;
        return;
    }

    // Ahora si abrimos el archivo, basicamente
    fd = open(candidate_real, O_RDONLY);
    if (fd < 0) {
        f.status = (errno == EACCES) ? HTTP_FORBIDDEN : HTTP_NOT_FOUND;
        *file = f;
        return;
    }

    // pedimos memoria para todo el archivo, y si no hay pues 500
    f.data = malloc((unsigned long)st.st_size + 1);
    if (f.data == NULL) {
        close(fd);
        f.status = HTTP_INTERNAL_ERROR;
        *file = f;
        return;
    }

    // leemos el archivo por pedazos hasta que este todo, o sea, completito
    while (total < (unsigned long)st.st_size) {
        n = read(fd, f.data + total, (unsigned long)st.st_size - total);
        if (n <= 0)
            break;
        total += (unsigned long)n;
    }

    close(fd);

    // si el read fallo, este, liberamos y devolvemos 500
    if (n < 0) {
        free(f.data);
        f.data = NULL;
        f.status = HTTP_INTERNAL_ERROR;
        *file = f;
        return;
    }

    f.size = total;
    f.status = HTTP_OK;
    *file = f;
}

/* y esta nomas libera la memoria y deja todo en cero, asi de facil */
void files_free(file_content *file)
{
    file_content f = *file;

    free(f.data);
    f.data = NULL;
    f.size = 0;
    *file = f;
}
