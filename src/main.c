#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "server.h"
#include "http.h"
#include "mime.h"
#include "files.h"

#define REQ_SIZE 8192

int main(void)
{
    int sd;
    int epfd;
    int nready;
    int nsd;
    struct epoll_event ev;
    struct epoll_event events[SERVER_MAX_EVENTS];

    printf("Iniciando MiniHTTPd...\n");

    // Primero arrancamos el server creando el socket
    sd = create_listen_socket(SERVER_PORT);
    if (sd < 0) {
        fprintf(stderr, "Fallo fatal al inicializar el servidor.\n");
        return EXIT_FAILURE;
    }

    // Ahora creamos el epoll, que es como el que vigila todos los sockets
    epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("Error: epoll_create1");
        close(sd);
        return EXIT_FAILURE;
    }

    // Y metemos el socket del server en epoll para que, este, nos avise
    ev.events  = EPOLLIN;
    ev.data.fd = sd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev) < 0) {
        perror("Error: epoll_ctl");
        close(sd);
        close(epfd);
        return EXIT_FAILURE;
    }

    printf("Entrando al bucle de eventos (Presiona Ctrl+C para apagar)...\n");

    // Este es el bucle de eventos
    while (1) {
        nready = epoll_wait(epfd, events, SERVER_MAX_EVENTS, -1);

        if (nready < 0) {
            if (errno == EINTR)
                continue;
            perror("Error en epoll_wait");
            break;
        }

        for (int i = 0; i < nready; i++) {
            if (events[i].data.fd == sd) {
                nsd = accept(sd, NULL, NULL);

                if (nsd == -1) {
                    perror("Error al aceptar cliente");
                    continue;
                }

                no_bloqueante(nsd);

                ev.events  = EPOLLIN;
                ev.data.fd = nsd;

                if (epoll_ctl(epfd, EPOLL_CTL_ADD, nsd, &ev) == -1) {
                    perror("Error al añadir cliente a epoll");
                    close(nsd);
                } else {
                    printf("¡Cliente conectado y registrado!\n");
                }
            } else {
                nsd = events[i].data.fd;
                char buffer[REQ_SIZE] = {0};
                char header_buffer[1024];
                http_request_t req;
                int keep = 0;

                ssize_t bytes_read = read(nsd, buffer, sizeof(buffer) - 1);

                if (bytes_read > 0) {
                    printf("--- Petición HTTP recibida ---\n");

                    if (http_parse_request(buffer, &req) == 0) {
                        printf("Método extraído: [%s]\n", req.method);
                        printf("URI extraída:    [%s]\n", req.uri);
                        printf("Versión:         [%s]\n", req.version);

                        // El proyecto dice que todo lo que no sea GET va con 405, asi que bueno
                        if (strcmp(req.method, "GET") == 0) {
                            // 1. Buscamos el archivo y lo leemos
                            file_content file;
                            files_read_secure(req.uri, &file);

                            // 2. Respondemos segun lo que nos dijo files.c, digamos si salio bien o no
                            if (file.status == HTTP_OK) {
                                // Vemos el tipo MIME, y si piden "/" pues es text/html porque es el index.html
                                const char *mime = (strcmp(req.uri, "/") == 0) ? "text/html" : mime_type(req.uri);

                                keep = req.keep_alive;

                                // Armamos los encabezados, basicamente lo que va antes del archivo
                                http_build_headers(header_buffer, sizeof(header_buffer), HTTP_OK,
                                                   mime, (off_t)file.size, keep);

                                // Mandamos primero los encabezados y despues el archivo, o sea en ese orden
                                if (write(nsd, header_buffer, strlen(header_buffer)) < 0)
                                    perror("write");
                                if (write(nsd, file.data, file.size) < 0)
                                    perror("write");

                                printf("-> Enviado: %s (%zu bytes) [MIME: %s]\n", req.uri, file.size, mime);
                            }
                            else if (file.status == HTTP_NOT_FOUND) {
                                snprintf(header_buffer, sizeof(header_buffer),
                                         "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n<h1>404 Archivo no encontrado</h1>");
                                if (write(nsd, header_buffer, strlen(header_buffer)) < 0)
                                    perror("write");
                                printf("-> Error 404: %s no encontrado.\n", req.uri);
                            }
                            else if (file.status == HTTP_FORBIDDEN) {
                                snprintf(header_buffer, sizeof(header_buffer),
                                         "HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n<h1>403 Acceso Denegado</h1>");
                                if (write(nsd, header_buffer, strlen(header_buffer)) < 0)
                                    perror("write");
                                printf("-> Error 403: Intento de acceso denegado a %s.\n", req.uri);
                            }
                            else {
                                snprintf(header_buffer, sizeof(header_buffer),
                                         "HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\n\r\n<h1>500 Error Interno</h1>");
                                if (write(nsd, header_buffer, strlen(header_buffer)) < 0)
                                    perror("write");
                                printf("-> Error 500: Fallo interno al leer %s.\n", req.uri);
                            }

                            // 3. Y liberamos la memoria del archivo, porque si no, este, se acumula
                            files_free(&file);

                        } else {
                            // Si mandan POST, PUT o cosas asi, pues les decimos que no
                            const char *error_response =
                                "HTTP/1.1 405 Method Not Allowed\r\n"
                                "Connection: close\r\n\r\n";
                            if (write(nsd, error_response, strlen(error_response)) < 0)
                                perror("write");
                            printf("¡Método %s rechazado (405)!\n", req.method);
                        }
                    } else {
                        const char *error_response =
                            "HTTP/1.1 400 Bad Request\r\n"
                            "Connection: close\r\n\r\n";
                        if (write(nsd, error_response, strlen(error_response)) < 0)
                            perror("write");
                        printf("Error: Petición mal formada (400 Bad Request).\n");
                    }

                    printf("------------------------------\n");

                    if (!keep)
                        close(nsd);
                }
                else {
                    close(nsd);
                }
            }
        }
    }

    close(sd);
    close(epfd);
    return EXIT_SUCCESS;
}