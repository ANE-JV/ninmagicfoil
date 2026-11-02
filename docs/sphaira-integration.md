# Integración de Sphaira en NinMagicFoil

Fecha de revisión: 2 de noviembre de 2026.

**Estado: diseño y auditoría de fuentes. No es una implementación, una compilación ni una prueba en consola.**

Este documento identifica componentes reutilizables de Sphaira 1.0.9, los cambios necesarios para una interfaz NinMagicFoil con el comportamiento solicitado de Tinfoil y las pruebas que deben superar esos cambios. No afirma paridad completa con Tinfoil ni compatibilidad universal con juegos o firmware.

## Base examinada y alcance de la evidencia

El repositorio actual de Sphaira es [NaGaa95/sphaira](https://github.com/NaGaa95/sphaira). El repositorio anterior de ITotalJustice redirige allí. Se han leído archivos del **tag `1.0.9`** mediante GitHub, junto con las [notas de esa versión](https://github.com/NaGaa95/sphaira/releases/tag/1.0.9). El proyecto declara [GPL-3.0](https://github.com/NaGaa95/sphaira/blob/1.0.9/LICENSE). Una futura integración debe conservar su licencia, atribuciones y avisos de dependencias correspondientes.

La versión 1.0.9 incorpora cola Ownfoil con instalación en segundo plano, archivado de juegos, selección de idioma de lanzamiento y correcciones de restauración de partidas, MTP y WebDAV. Estas son funciones documentadas por el proyecto; no son resultados de pruebas realizadas en la consola del usuario.

La equivalencia exacta con Tinfoil debe contrastarse con el inventario de sus pantallas, opciones y documentación. La tabla siguiente identifica posibilidades y carencias de Sphaira, sin sustituir ese inventario.

## Mapa de componentes

| Área | Componente reutilizable de Sphaira 1.0.9 | Trabajo para NinMagicFoil |
| --- | --- | --- |
| Pantalla principal | Widgets, listas, cuadros, controles, navegación y dibujo NanoVG/deko3d. | Crear navegación lateral, tabla de títulos, columnas, filtros, panel de información y atajos según la referencia de Tinfoil. Cambiar el tema no transforma por sí solo la estructura de menús. |
| Instalación | YATI dispone de entradas desde archivo, fuente, contenedor y colección; contenedores NSP/XCI y tratamiento de contenido NCZ. | Adaptar progreso, errores, confirmaciones y selección de destino; verificar cada formato y origen con pruebas. |
| Catálogo Ownfoil | Descubrimiento, cuentas, direcciones local/remota, catálogo por contenido, versiones, DLC, imágenes y selección de componentes. | Integrarlo en la interfaz nueva y verificar el protocolo del servidor del usuario. |
| Tiendas y orígenes de Tinfoil | Fuentes HTTP y varios sistemas de archivos de red. | La API Ownfoil no equivale a un lector universal del índice JSON/TFL o a un cliente NUT. Implementar y comprobar los adaptadores que requiera el inventario de Tinfoil. |
| Cola | Ejecución secuencial en segundo plano, cancelación, eliminación de pendientes y limpieza de terminados. | Nueva vista; persistencia, pausa, reanudación manual, reordenación o reintento requieren trabajo específico si forman parte del comportamiento requerido. |
| Nombres y metadatos | Caché libnxtc, NACP de almacenamiento, tratamiento de NACP comprimidos, idiomas y datos Ownfoil. | Unificar resolución para juegos instalados, catálogo, archivos y tickets; preservar identificadores como respaldo cuando falte información. |
| FTP/MTP/USB | Servicios, fuentes y menús de instalación por flujo con cancelación. | Integrar controles y estados sin interrumpir transferencias al cambiar de pantalla. Cada transporte necesita comprobación independiente. |
| Actualizador propio | Consulta GitHub, notas, descarga temporal, comprobación NACP/versión, respaldo y recuperación ante errores de copia. | Canal y rutas de NinMagicFoil, identidad propia, selección estricta de artefactos y validación de integridad del paquete. |
| Temas | INI de colores/texturas, herencia, música y recursos RomFS. | Tema y logo NinMagic; importar temas de Tinfoil requiere un formato compatible o un traductor específico. |
| Tickets | Funciones ES de importación, enumeración y lectura empleadas por instalación/exportación. | Desarrollar una vista de gestión y las operaciones necesarias. No se ha encontrado un menú dedicado equivalente al de Tinfoil. |
| Partidas | Enumeración por perfiles, copia, restauración, elección de destino y respaldo previo a restaurar. | Adaptar vistas y extraer servicios cuando la lógica esté dentro del menú. Mantener confirmaciones y recuperación. |
| Juegos instalados | Listado, lanzamiento, metadatos, contenidos, exportación, eliminación y archivado. | Reorganizar la experiencia y mantener claramente diferenciadas las operaciones. |

## Archivos y puntos de integración

Todas las referencias siguientes corresponden al tag 1.0.9.

### Interfaz, navegación y temas

- [`main_menu.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/menus/main_menu.hpp) y [`main_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/main_menu.cpp): `MainMenu` es una clase final y administra menús central, izquierdo y derecho. La tabla de fábricas empieza alrededor de la línea 304. Una pantalla principal NinMagic basada en `Widget` permite una organización propia.
- [`menu_base.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/menus/menu_base.hpp), [`types.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/types.hpp), [`list.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/list.cpp) y [`sidebar.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/sidebar.cpp): primitivas de controles, acciones, estilos y listas.
- [`app.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/app.cpp): carga de temas en `LoadThemeInternal` y `LoadTheme`; actualización y dibujo globales de la cola. Los temas incluidos están en [`assets/romfs/themes`](https://github.com/NaGaa95/sphaira/tree/1.0.9/assets/romfs/themes).
- [`sphaira/CMakeLists.txt`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/sphaira/CMakeLists.txt): módulos, dependencias fijadas y opciones de compilación de servicios y protocolos. SFTP está desactivado por defecto; no debe anunciarse como incluido sin comprobar la configuración resultante.

### Instalación, fuentes y cola

- [`yati.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/yati/yati.hpp): entradas `InstallFromFile`, `InstallFromSource`, `InstallFromContainer` e `InstallFromCollections`, alrededor de las líneas 89–92.
- [`yati.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/yati/yati.cpp): organización de contenidos y ejecución de instalaciones. No debe inferirse compatibilidad con todo contenido a partir de que exista el parser.
- [`source/base.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/yati/source/base.hpp) y [`source/http.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/yati/source/http.hpp): contrato de lectura y fuente HTTP; utilizan offsets y tamaños amplios que deben mantenerse sin conversiones a 32 bits.
- [`progress_box.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/progress_box.hpp): combina widget, hilo de trabajo, progreso y cancelación. YATI recibe un puntero a este tipo; por tanto, el instalador no está completamente desacoplado de la interfaz.
- [`install_queue.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/install_queue.hpp) y [`install_queue.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/install_queue.cpp): la cola conserva callbacks en memoria, con estados pendiente, ejecutando, terminado, fallido y cancelado. Al cerrar se cancela el trabajo actual y se descartan los pendientes. No es una cola persistente entre reinicios.

### Ownfoil y nombres

- [`ownfoil_api.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/utils/ownfoil_api.hpp) y [`ownfoil_api.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/utils/ownfoil_api.cpp): handshake mediante `OPTIONS`, capacidades del servidor y peticiones a `api/graphql`. La compatibilidad debe comprobarse contra el servidor real; no basta con que su URL funcione en Tinfoil.
- [`ownfoil.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/ownfoil.cpp) y [`ownfoil_title.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/ownfoil_title.cpp): catálogo y detalles. En el segundo archivo, alrededor de las líneas 973–1052, se agrupan los contenidos elegidos por descarga y se envían a la cola. Esta lógica puede reutilizarse con otra presentación.
- [`title_info.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/title_info.cpp): normalización de NACP comprimidos y resolución mediante caché, lectura manual y almacenamiento. Resuelve idioma y aplica información disponible; no demuestra que siempre exista un nombre válido para cualquier identificador.

### Transferencias, juegos, tickets y partidas

- [`install_stream_menu_base.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/ui/menus/install_stream_menu_base.hpp), [`ftp_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/ftp_menu.cpp), [`mtp_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/mtp_menu.cpp), [`ftpsrv_helper.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ftpsrv_helper.cpp) y [`haze_helper.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/haze_helper.cpp): entrada de datos por flujo, callbacks y ciclo de vida de servicios.
- [`game_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/game_menu.cpp), [`game_meta_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/game_meta_menu.cpp), [`game_nca_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/game_nca_menu.cpp) y [`storage_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/storage_menu.cpp): biblioteca instalada, contenidos y almacenamiento.
- [`es.hpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/include/yati/nx/es.hpp): funciones de tickets alrededor de las líneas 182–223. La presencia de estas funciones no implica que exista toda la gestión interactiva de tickets de Tinfoil.
- [`save_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/save_menu.cpp): copia y restauración. Parte de la lógica está integrada en métodos del menú y debe extraerse si se necesita otra vista sin duplicar las operaciones.

### Actualizador

[`main_menu.cpp`](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/main_menu.cpp) contiene la consulta GitHub, lectura de la publicación y `InstallUpdate`, a partir de la línea 372. El código revisado usa el repositorio de Sphaira, comprueba el nombre Sphaira en NACP y considera rutas como `/hbmenu.nro` y `/switch/sphaira/...`.

NinMagicFoil necesita identidad, destinos y canal de publicación propios. Debe validar el artefacto completo antes de sustituir la aplicación y conservar un respaldo recuperable. La validación de nombre y versión NACP de Sphaira no demuestra por sí sola una verificación SHA-256 del paquete; esa garantía debe implementarse y probarse explícitamente si se anuncia.

## Separación propuesta entre interfaz y servicios

La primera integración puede conservar los servicios de Sphaira dentro del proyecto y colocar una interfaz NinMagic por encima. No es necesario convertir todo Sphaira en una biblioteca antes de conseguir una aplicación comprobable.

1. **Presentación:** pantalla principal, tabla, panel lateral, detalles, opciones, cola y diálogo de progreso. La vista muestra estados y solicita acciones; no manipula directamente archivos instalados.
2. **Servicios de aplicación:** catálogo, biblioteca instalada, tareas de instalación, gestión de archivos, partidas y actualizador. Mantienen la operación aunque la vista cambie.
3. **Adaptadores:** Ownfoil, otros formatos de tienda que se decidan soportar, transporte HTTP/FTP/MTP/USB y metadatos. Cada adaptador declara sus capacidades y devuelve errores distinguibles.
4. **Motor y plataforma:** YATI, contenedores, lectura de fuentes, servicios de la consola y almacenamiento.

El primer punto de extracción es el progreso/cancelación hoy incluido en `ProgressBox`. Puede conservarse temporalmente con una presentación nueva; a medio plazo conviene un contrato que publique progreso y errores sin conocer widgets. La cola persistente necesitaría descripciones de trabajos serializables, no callbacks que capturen un menú o sus datos temporales.

Las acciones cuyo trabajo vive dentro de un menú deben extraerse antes de duplicar la pantalla. Esto es especialmente relevante para partidas y transferencias: destruir un menú no debe dejar callbacks apuntando a objetos inexistentes ni interrumpir una tarea sin informar al usuario.

## Pruebas requeridas para contenido moderno

Las pruebas deben distinguir tres niveles: verificaciones en PC, compilación para Switch y ejecución en consola. Ninguno sustituye a los otros. La matriz siguiente es un plan pendiente, no una lista de comprobaciones superadas.

| Área | Casos mínimos | Criterio de aceptación |
| --- | --- | --- |
| PFS0 y tamaños de 64 bits | Entradas y offsets antes, en y después de 2 GiB y 4 GiB; CNMT al principio y al final; tamaños grandes, ceros, truncados y sumas que desbordan. Fixtures sintéticos o archivos dispersos permiten probar límites sin redistribuir juegos. | El parser conserva offsets/tamaños exactos y rechaza tablas fuera de límites antes de reservar memoria o leer contenido. |
| HTTP Range | Lecturas que cruzan 2 GiB/4 GiB; rangos del encabezado y del CNMT; respuestas 206 correctas, 200 inesperadas, 416, `Content-Range` incorrecto, descarga corta, redirección y pérdida de conexión. | Los bytes recibidos coinciden con el archivo local; una respuesta inválida produce un error de transporte reconocible y no se interpreta como metadatos válidos. |
| Identidad del archivo | Tamaño y hash del mismo archivo en origen, copia usada por DBI y respuesta HTTP; rangos de control distribuidos por el archivo. | Se demuestra que ambas vías usan exactamente el mismo contenido. Un nombre de archivo igual no basta. |
| CNMT | Metadatos válidos, ausentes, truncados y con contadores incoherentes; base, actualización y DLC; contenedores con varios contenidos. | Se identifica la fase y el contenido que falla. Se rechazan accesos fuera de límites y se instala únicamente la selección confirmada. |
| Formatos | NSP, NSZ, XCI y XCZ, en SD, HTTP/Ownfoil y los transportes habilitados. Incluir variantes grandes y fallos de descompresión. | Cada combinación declarada compatible supera lectura, instalación y verificación en consola. Soportar una extensión no demuestra soportar todas sus variantes. |
| NACP comprimidos | Datos normales y comprimidos, tamaño declarado incorrecto, truncamiento y texto UTF-8 válido/inválido. | Se obtienen nombres legibles sin interpretar bytes comprimidos como texto; los datos inválidos producen una alternativa visible y estable. |
| Idiomas y nombres | Español, inglés y al menos un idioma de escritura no latina; idioma solicitado ausente; solo Title ID; discrepancia entre archivo, caché y servidor. | Hay una prioridad documentada de fuentes e idioma. Se conserva el identificador y no se atribuye un nombre de otro título. |
| Actualizador | Paquete correcto, hash incorrecto, nombre/versión/arquitectura incorrectos, ZIP incompleto, falta de espacio, fallo de escritura y reinicio durante cada fase. | El paquete inválido nunca sustituye la aplicación. Cada interrupción termina con la versión anterior utilizable o una recuperación explícita y comprobada. |
| Cola y cancelación | Salir de detalles mientras instala, cancelar descarga/escritura, fallo de un trabajo, siguiente trabajo, cierre y reinicio si hay persistencia. | Sin referencias a vistas destruidas, estados falsos de éxito ni contenidos parciales presentados como instalación terminada. |
| Regresión de datos | Copia/restauración de partidas de prueba, archivado, eliminación de contenido y cambio de destino. | Las operaciones afectan solo al elemento confirmado y se comprueba la conservación de los datos que deben permanecer. |

Los ensayos de corte de energía y de restauración deben diseñarse primero con fallos simulados y datos de prueba. La evidencia debe registrar versión del código, formato, origen, tamaño, configuración y resultado, sin publicar credenciales, claves ni datos privados de la consola.

## Caso aportado por el usuario: archivo base de 14 GB

El usuario informa de que el archivo base de EA Sports FC 27, mostrado como aproximadamente 14 GB, se instala con DBI y falla desde su servidor Ownfoil usando Tinfoil. Las capturas muestran `Failed to read NSP metadata` y `failed to open cnmt`.

Esto justifica investigar la lectura remota y la interpretación de metadatos. **No demuestra que el tamaño sea la causa, que el archivo esté dañado, que falten claves ni que Sphaira ya lo corrija.** Tampoco demuestra que las dos rutas utilicen una copia idéntica del archivo.

Para dar el fallo por resuelto deben cumplirse estos puntos:

1. Identificar el archivo exacto mediante tamaño y hash, sin publicarlo ni incluirlo en el repositorio.
2. Registrar las versiones de consola, firmware, entorno, servidor y aplicaciones utilizadas en la comparación.
3. Verificar encabezado PFS0 y rangos HTTP, especialmente el CNMT y offsets superiores a 4 GiB cuando los haya.
4. Reproducir el fallo con registros que distingan transporte, contenedor, metadatos y servicios de instalación.
5. Instalar la misma copia desde NinMagicFoil por el servidor del usuario y verificar el resultado real en consola.
6. Repetir casos de regresión de base, actualización, DLC y formatos comprimidos para delimitar qué se ha corregido.

No se propone cambiar claves, desactivar comprobaciones ni modificar requisitos del sistema como solución asumida. El arreglo debe derivarse de una causa reproducida y debe conservar los errores que indiquen incompatibilidad real.

## Condiciones para declarar una versión utilizable

- El código compila con dependencias identificadas y conserva los avisos correspondientes.
- El nombre y logo NinMagic aparecen en la aplicación, su icono y metadatos; las atribuciones técnicas se conservan donde corresponde.
- Las funciones publicadas coinciden con las implementadas; las pendientes se identifican expresamente.
- La navegación y cada operación se verifican con controles físicos, táctiles cuando se soporten y errores representativos.
- El actualizador usa el canal de NinMagicFoil y supera las pruebas de integridad y recuperación.
- El caso de 14 GB solo se anuncia como corregido tras comprobarlo con el archivo y servidor del usuario.
- La afirmación de paridad con Tinfoil solo se realiza cuando el inventario completo de funciones tenga evidencia de implementación y prueba. No se sustituye por una promesa de que todos los juegos funcionarán.
