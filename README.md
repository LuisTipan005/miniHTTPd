# MiniHTTPd

Servidor HTTP/1.1 básico para Linux, escrito en C sin bibliotecas HTTP externas. Escucha en el **puerto 2017** y sirve los archivos de la carpeta `www/`. Atiende varios clientes con un solo hilo usando `epoll` y sockets no bloqueantes, y mantiene la conexión abierta (keep-alive) cuando el cliente lo pide.

## Requisitos

- Linux (usa `epoll`)
- gcc y make
- Para probar: curl y nc (netcat)

## Compilar y ejecutar

```sh
make            # genera ./minihttpd (las advertencias se tratan como errores)
./minihttpd     # escucha en el puerto 2017 y sirve ./www
make debug      # genera ./minihttpd_debug con AddressSanitizer y UBSan
make clean      # borra build/ y los ejecutables
```

El servidor debe ejecutarse desde la carpeta del proyecto, porque busca `www/` con una ruta relativa.

Abrir en el navegador: <http://localhost:2017/>. `Ctrl+C` detiene el servidor.

## Estructura

```
include/   server.h  http.h  files.h  mime.h
src/       main.c  server.c  http.c  files.c  mime.c
www/       index.html  style.css  image.png
tests/     test_parser.c  run_tests.sh
```

## Módulos

| Módulo | Responsabilidad |
|---|---|
| `main.c` | Crea el socket de escucha y la instancia de `epoll`, y ejecuta el bucle de eventos. Acepta clientes nuevos, lee la petición, decide la respuesta (200, 400, 403, 404, 405 o 500), la envía y cierra la conexión si no es keep-alive. |
| `server.c` | `no_bloqueante(sd)`: pone un socket en modo no bloqueante con `fcntl`. `create_listen_socket(port)`: `socket()`, `SO_REUSEADDR`, `bind()` en todas las interfaces, modo no bloqueante y `listen()`. Devuelve el socket `sd`. |
| `http.c` | `http_parse_request()`: extrae método, URI y versión de la primera línea, exige que la petición termine en `\r\n\r\n`, acepta solo HTTP/1.1 y HTTP/1.0, exige el encabezado `Host` en HTTP/1.1 y decide `keep_alive`. `http_build_headers()`: arma la línea de estado y los encabezados `Content-Type`, `Content-Length` y `Connection`. |
| `files.c` | `files_read_secure()`: convierte la URI en una ruta dentro de `www/` (`/` se sirve como `index.html`), la valida y lee el archivo completo a memoria. Devuelve el contenido y el código de estado en un `file_content`. `files_free()`: libera la memoria del archivo. |
| `mime.c` | `mime_type()`: elige el `Content-Type` según la extensión del archivo. |

## Recorrido de una petición

1. `epoll` avisa que hay datos en el socket del cliente.
2. `main.c` lee la petición (hasta 8192 bytes) con un solo `read()`.
3. `http.c` la analiza. Si es inválida se responde 400 y se cierra.
4. Si el método no es GET se responde 405 y se cierra.
5. `files.c` busca y lee el archivo. Si falla devuelve 403, 404 o 500.
6. `mime.c` elige el tipo y `http.c` arma los encabezados.
7. `main.c` envía los encabezados y el contenido del archivo.
8. Si es keep-alive el socket sigue en `epoll`. Si no, se cierra.

## Concurrencia

Un solo hilo con `epoll` en modo level-triggered. El socket de escucha y los clientes están en modo no bloqueante. `epoll_wait` espera sin límite de tiempo (`-1`) y cada evento se atiende completo antes de pasar al siguiente.

## Seguridad

- **Directory traversal:** si la URI contiene `..` se responde 403. Además la ruta se resuelve con `realpath()` y se comprueba que empiece con la ruta real de `www/` seguida de `/` o del final de la cadena.
- **Enlaces simbólicos:** `realpath()` los resuelve antes de la comprobación, así que un enlace que apunte fuera de `www/` da 403.
- **Solo archivos regulares:** un directorio da 403.
- **Límites de campos:** los campos de la petición se leen con `sscanf` con anchos máximos (método 15, URI 255, versión 15 caracteres).
- **Buffers:** las cadenas se arman con `snprintf` o `memcpy` después de comprobar longitudes.

## Tipos MIME

| Extensión | Content-Type |
|---|---|
| `.html` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.png` | `image/png` |
| `.jpg` | `image/jpeg` |
| otros | `application/octet-stream` |

La extensión se compara sin distinguir mayúsculas.

## Códigos de estado

| Código | Situación | Conexión |
|---|---|---|
| 200 | Archivo enviado | según keep-alive |
| 400 | Petición mal formada, versión no soportada, falta `Host` en HTTP/1.1 | se cierra |
| 403 | URI con `..`, fuera de `www/`, sin permiso, no es archivo regular | se cierra |
| 404 | El archivo no existe | se cierra |
| 405 | Método distinto de GET | se cierra |
| 500 | Error interno (por ejemplo, sin memoria) | se cierra |

## Pruebas

Con el servidor corriendo:

```sh
curl -i http://localhost:2017/                         # 200
curl -i http://localhost:2017/nope                     # 404
curl -i -X POST http://localhost:2017/                 # 405
printf 'GET / HTTP/1.1\r\n\r\n' | nc localhost 2017    # 400 (sin Host)
```

Resultados de la última ejecución: compila sin errores con `-Werror` y las pruebas de 200, 404, 405 y 400 pasan.

## Limitaciones conocidas

- **Archivos grandes:** el contenido se envía con un solo `write()` sobre un socket no bloqueante. Si el socket no acepta todo de una vez, el resto no se envía y el cliente se queda esperando. En la prueba con un archivo de 4 MB la descarga no terminó.
- **Lectura de la petición:** se hace con un solo `read()`. Si llega partida en varios paquetes, se trata como mal formada (400).
- **Respuestas de error:** las 403, 404 y 500 no llevan `Content-Length`, y la 405 no lleva el encabezado `Allow`.
- **Timeout de inactividad:** no existe. `SERVER_IDLE_TIMEOUT` está definido en `server.h` pero no se usa.
- **Pruebas desactualizadas:** `tests/test_parser.c` usa una versión anterior de `http.h` (`http_parse_result_t`, `http_parse_request_line`) y no compila con el código actual, por eso el Makefile ya no tiene el objetivo `make test`. `tests/run_tests.sh` tampoco está actualizado.
