# Estudio de la consola conectada

Fecha: 2 de noviembre de 2026. Inspección de lectura; ninguna instalación, borrado o cambio de configuración realizado en la consola.

## Evidencia obtenida

- Windows detecta la Switch por USB y la interfaz de archivos MTP funciona.
- Las ubicaciones expuestas son `firmware`, `Instalados`, `Instalar`, `Tarjeta micro SD`, `Sistema` y `Usuario`.
- Se ha enumerado `sdmc:/switch/tinfoil/`. Contiene el ejecutable NRO, opciones, ubicaciones, caché, bases de títulos, licencias y una carpeta de temas sin entradas visibles.
- La configuración leída tiene idioma español, región US, tema automático y MTP/NUT activados. Son valores guardados; no demuestran por sí solos el estado de cada servicio en ejecución.
- Existen referencias guardadas a las vistas Installed, File Browser, New Games, New Updates, New DLC, Recommended, Shared Saves, Queue, Tickets, Title Keys, Console, Incomplete, XCI, Homebrew Store, Firmware y Payloads, además de categorías de emulación. No se han confundido con un inventario visual de pantallas abiertas.
- La interfaz USB adicional identificada como Tinfoil tiene código de problema 28 de Windows. MTP funciona de forma independiente. No se ha instalado ni cambiado ningún controlador.

## Límites de la observación

MTP proporciona acceso a archivos; esta conexión no ha ofrecido una vista de vídeo ni control remoto de los menús. Por ello no se han comprobado todavía los estados de foco, menús emergentes, atajos, animaciones y diálogos de todas las pantallas de Tinfoil 20 en la consola.

Las capturas públicas de la documentación sirven de referencia histórica. Los datos de configuración y las cadenas del instalador tampoco demuestran que una función esté habilitada o funcione correctamente en la aplicación instalada.

## Caso de instalación pendiente

El usuario indica que DBI instala el NSP base de aproximadamente 14 GB y que Tinfoil falla desde Ownfoil con mensajes sobre metadatos NSP/CNMT. No se ha intentado instalar, descargar entero ni modificar ese juego. La causa todavía no está identificada: la evidencia disponible no permite atribuirla a Ownfoil ni afirmar que una base diferente ya lo corrija.

## Conservación de datos

La copia local de opciones contiene campos privados y se conserva solo bajo `.local/`, excluido de Git. El inventario publicado no incluye valores de credenciales, huellas, claves API, ubicaciones remotas ni listas de juegos del usuario. El estudio no requiere publicar el NRO ni los recursos de Tinfoil.
