# Inventario de referencia de Tinfoil para NinMagicFoil

Fecha de revisión: 2 de noviembre de 2026, Europe/Madrid.

Este documento delimita qué se ha podido comprobar para reproducir la experiencia de Tinfoil sobre una base abierta. No certifica paridad completa con Tinfoil 20 ni compatibilidad con todos los juegos. La configuración leída de la consola, la documentación histórica y las cadenas de un instalador son evidencias distintas.

## Evidencias y límites

| Código | Evidencia | Qué permite afirmar |
|---|---|---|
| DOC | Documentación pública del autor: portada, descargas, red, índices, DRM, MTP y USB. | Describe contratos y capacidades publicadas; no todas las páginas están versionadas para 20. |
| IMG20 | Seis capturas de la documentación oficial cuyos nombres corresponden al 23-02-2020. | Permiten estudiar composición, navegación y controles históricos; no confirmar la interfaz de 2026. |
| BIN20 | Inspección estática en memoria del paquete oficial `Tinfoil-20.0-v2-Self-Installer.zip`. | Confirma recursos y cadenas presentes en ese paquete. Una cadena puede estar sin uso, deshabilitada o conservada por compatibilidad. |
| CFG | Nombres de ventanas hallados por la inspección principal en configuración guardada de la consola conectada. | Confirma nombres persistidos. No confirma que esas ventanas estén visibles, accesibles o completas en la sesión actual. |
| MTP | Enumeración actual del dispositivo conectado. | Confirma los almacenes MTP que expone; no proporciona vídeo ni control de la interfaz de Switch. |
| USER | Capturas y descripción aportadas por el usuario. | Confirma los errores mostrados y el resultado que el usuario comunicó con DBI. |

El ZIP BIN20 tiene SHA-256 `79f9fa1b3dc270d029f7b30d79440a9840826fa422003fc3dc59b4482c34a992`. Su NRO declara una parte ejecutable de 995.328 bytes y contiene en RomFS un NSP de 16.863.960 bytes. El NACP identifica el ejecutable como instalador. Las cadenas legibles analizadas pertenecen a ese ejecutable y sus bibliotecas compartidas; no se ha ejecutado ni descifrado el NSP interno.

La documentación tiene limitaciones concretas: ejemplos con versión 7.00, capturas de 2020, una página USB que aún contiene la plantilla de MkDocs y una descripción MTP con un límite histórico de 4 GB. Ninguno de esos detalles debe convertirse automáticamente en requisito o restricción de Tinfoil 20. La web no ofrece por sí sola una especificación completa y vigente de todas las pantallas.

## Menús identificados en la configuración de la consola

La inspección principal comunicó estos nombres, conservados aquí sin datos personales ni contenido de la biblioteca:

| Grupo de estudio | Ventanas guardadas (CFG) |
|---|---|
| Biblioteca | `Installed`, `New Games`, `New Updates`, `New DLC`, `Recommended`, `XCI` |
| Orígenes y archivos | `File Browser`, `LocationList` |
| Trabajos | `Queue`, `Incomplete` |
| Otros contenidos | `Shared Saves`, `Homebrew Store`, `Payloads`, `Firmware` |
| Administración e información | `Tickets`, `Title Keys`, `Console` |
| Sistemas de emulación | `GBA`, `Game Gear`, `Gameboy`, `N64`, `NDS`, `NES`, `Play Station`, `PlayStation Portable`, `SG-1000`, `SNES`, `Sega CD`, `Sega Dreamcast`, `Sega Genesis`, `Sega Master System` |

`Options` y `Help` aparecen en BIN20 y en imágenes históricas, aunque no forman parte de la lista CFG comunicada. `Incognito` y `Dump Title Keys` son rótulos visibles en IMG20; no se ha confirmado su presencia actual con esos nombres. `About` y distintos diálogos de instalación también aparecen como cadenas. Algunos apartados pueden depender de ubicaciones configuradas o contenido disponible: no se debe deducir un menú fijo únicamente de esta lista.

## Composición y navegación visual

Las capturas IMG20 muestran un lienzo de 1280 × 720: cabecera con marca y contexto, barras de almacenamiento, reloj e indicadores; panel lateral de unos 375 píxeles con iconos; área principal desplazable; pie con acciones y perfil. La biblioteca utiliza una rejilla de seis columnas en una de las vistas. El explorador usa tabla de nombre y tamaño. La ficha combina imágenes y descripción. Las partidas muestran perfil y tabla de copias; los trucos, una lista de conmutadores.

Controles históricos observados, pendientes de cotejar en Tinfoil 20:

| Pantalla IMG20 | Acciones visibles |
|---|---|
| Biblioteca instalada | `+` búsqueda; L/R perfil anterior/siguiente; X refrescar; B salir; A aceptar. |
| Raíces del explorador | `−` nueva ubicación; L/R reordenar; Y borrar; X editar; A abrir. |
| Ficha de juego | L copias de partidas; R menú de instalación; Y trucos; B volver; A iniciar. |
| Copias de partidas | ZL copia de todas; L copia del juego; R perfil siguiente; Y borrar; B volver; A restaurar. |
| Opciones y trucos | Filas seleccionables y B para volver. |

Las capturas USER del explorador muestran instalación múltiple, copiar, borrar, renombrar y abrir en su pie. Falta observar en la consola actual el foco entre paneles, cambio de vista, ordenación, filtros, teclado, paginación, selección múltiple, gestos táctiles, cuadros de confirmación y estados vacíos o de error. Reproducir esa experiencia requiere trabajo en la interfaz; un tema de Sphaira no la convierte automáticamente en Tinfoil.

Fuentes visuales: [biblioteca](https://blawar.github.io/tinfoil/images/2020022313192700.jpg), [explorador](https://blawar.github.io/tinfoil/images/2020022313220100.jpg), [opciones](https://blawar.github.io/tinfoil/images/2020022313222600.jpg), [ficha](https://blawar.github.io/tinfoil/images/2020022313032000.jpg), [trucos](https://blawar.github.io/tinfoil/images/2020022313175800.jpg), [partidas](https://blawar.github.io/tinfoil/images/2020022313030700.jpg).

## Inventario funcional candidato

Esta tabla se deriva principalmente de BIN20 y CFG. Identifica trabajo que habría que observar y validar; no declara que cada función esté operativa hoy. Los nombres se resumen y agrupan para especificar comportamiento, sin copiar los textos de ayuda.

| Área | Funciones o variantes identificadas | Pendiente de verificar |
|---|---|---|
| Biblioteca | Instalados; títulos nuevos; actualizaciones; DLC; recomendados; XCI; mostrar instalados; mostrar solo títulos con archivo; búsqueda, reinicio y continuación de búsqueda; vistas y ordenación. | Condiciones de aparición, campos de filtro, orden por defecto y persistencia. |
| Ficha | Nombre, Title ID, autor/editor, versión, región, fecha, tamaño, categoría, jugadores, clasificación, descripción, imágenes; inicio y acceso a submenús. | Campos realmente visibles, idioma, fuentes y alternativa cuando faltan metadatos. |
| Instalación | Selección de destino; instalación individual/múltiple; última versión; incluir actualización y DLC; contenidos de XCI; reinstalación de NCA; ticket solo; streaming; copia local de lo instalado; borrado posterior. | Valores predeterminados, selección de contenidos, cancelación, recuperación y trato de archivos malformados. |
| Validación | Avisos de firma NCA, metadatos y firmware requerido; opciones de contenido sin firma, conversión criptográfica y comprobación flexible de versión. | Semántica exacta y disponibilidad por firmware. No equivalen a garantizar que un juego arranque. |
| Cola | Trabajos en segundo plano; pendientes, en curso, incompletos, fallidos y terminados; velocidad y estado; concurrencia configurable. | Pausa/reanudación, reintento, prioridades, limpieza y persistencia tras cierre. |
| Explorador | Añadir/editar/reordenar/eliminar ubicaciones; abrir; copiar/pegar; borrar/renombrar; fijar rutas para escaneo; asignar directorios de copias de juegos o partidas. | Diferencias entre raíz, carpeta y archivo; permisos y errores por protocolo; escaneo recursivo real. |
| Gestión instalada | Iniciar, desinstalar, exportar a NSP y restablecer versión requerida. | Alcance sobre base/actualizaciones/DLC y conservación de partidas. |
| Partidas | Copia individual o de todas; perfiles; restauración; comentarios; slots; copias ZIP; comparación para evitar duplicados; copia previa a restauración; destino local o remoto; copias compartidas. | Compatibilidad de ZIP, tipos de guardado, automatismos, conflictos de perfil y servicios externos. |
| Trucos | Consulta y conmutadores por juego/versión; base local de trucos incluida en el ZIP. | Identificación por build, escritura efectiva, compatibilidad con CFW y fuente de actualizaciones. |
| Homebrew y ROMs | Catálogo de homebrew; inicio de NRO/contenido; integración de núcleos RetroArch; descarga previa de ROM remota; secciones por sistema. | Formatos, rutas de núcleos, parámetros de inicio, caché y requisitos externos. |
| Cartuchos y USB | Aviso al insertar cartucho; instalación; desactivación del aviso; almacenamiento USB; NUT/USBFS; MTP. | Protocolos y unidades admitidos por la versión concreta; límites de tamaño actuales. |
| Tickets y exportación | Tickets comunes/personalizados; importación; exportación de títulos y datos relacionados; identificadores de contenido. | Operaciones reales disponibles y tratamiento de datos sensibles. |
| Sistema | Información de consola, memoria y firmware; payloads; funciones de firmware y gestión de módulos/temas del sistema. | Menús, requisitos y operaciones realmente soportadas en el hardware actual. |
| Funciones heredadas | Cadenas de restablecimiento de fábrica, borrado de datos personales, enlace de cuenta y eliminación de software antiguo. | Pueden ser restos de biblioteca. No se deben implementar ni probar por inferencia de una cadena. |
| Aplicación | Temas, música, idioma/región, control parental, nivel de registro, actualización propia, servicios, ayuda y credenciales. | Campos, límites, estado después de reiniciar y modo applet frente a aplicación. |

## Opciones identificadas para cotejo

BIN20 permite preparar una revisión por grupos. La presencia de un rótulo no proporciona su rango de valores ni su valor por defecto.

- Apariencia y catálogo: tema, idioma, región, música, control parental, descarga/extracción de iconos, precarga de iconos/imágenes/DLC/base de datos, títulos instalados y títulos con archivo disponible.
- Red y rendimiento: tiempo de conexión, número de descargas simultáneas, streaming, servicios MTP y NUT, servidor FTP, frecuencias durante instalación o navegación, nivel de registro.
- Instalación: actualización más reciente, todos los DLC, contenidos internos de XCI, reinstalación, ticket solo, comprobaciones de firma/firmware, copia posterior y borrado posterior.
- Copias: directorio de juegos, directorio de partidas y uso compartido del directorio entre perfiles.
- Integraciones: cuenta Tinfoil, claves de API de proveedores y comprobación/descarga de actualización de la propia aplicación.

No se han activado estas opciones ni ejecutado acciones de instalación, borrado, restauración, firmware o cuentas durante esta investigación.

## Contratos remotos publicados

**Red (DOC).** El autor documenta NUT, HTTP/HTTPS con listados HTML o JSON y lectura por rangos, autenticación básica o certificado cliente, FTP/FTPS, SMB, Google Drive, 1Fichier y Dropbox. También describe índices de archivos fragmentados y lectura por desplazamientos. Algunas solicitudes de directorio incluyen idioma, versión, identidad de usuario, tema y autenticadores de URL/dominio. Los proveedores y sus mecanismos de acceso pueden haber cambiado desde la publicación: hay que comprobar cada integración antes de declararla compatible. [Network Install](https://blawar.github.io/tinfoil/network/).

**Índices (DOC).** El formato documentado admite archivos, carpetas, tamaños, mensajes, nombre alternativo, metadatos por título, ubicaciones y cabeceras personalizadas. También contempla versión mínima del cliente, certificados, claves de proveedores y restricciones de temas. Estos campos son un contrato de interoperabilidad distinto de la API nativa Ownfoil de Sphaira. Que ambos programas usen HTTP no implica que interpreten el mismo catálogo. [Custom Index](https://blawar.github.io/tinfoil/custom_index/).

**Índices cifrados (DOC).** La documentación describe cifrado simétrico con una clave envuelta mediante RSA para Tinfoil. La clave pública publicada sirve para preparar contenido destinado al cliente original; no entrega a un fork su clave privada. No se ha demostrado que NinMagicFoil pueda abrir esos índices. [DRM](https://blawar.github.io/tinfoil/drm/).

**MTP (DOC y MTP).** La documentación describe vistas de instalaciones, firmware, índice remoto y particiones. Su límite publicado de tamaño es histórico y no se ha confirmado en v20. En la consola conectada se han enumerado actualmente `firmware`, `Instalados`, `Instalar`, `Tarjeta micro SD`, `Sistema` y `Usuario`. La enumeración no prueba las operaciones de escritura de cada almacén. [MTP](https://blawar.github.io/tinfoil/mtp/).

**Actualización (DOC/BIN20).** La página de descarga distingue self-installer y applet e indica que la variante applet no se actualiza automáticamente. BIN20 contiene ayuda para actualizar y finalizar mediante salida limpia. Deben observarse el flujo actual y su formato de distribución; el nuevo proyecto necesitará un canal propio. [Descargas](https://tinfoil.media/Download).

**USB (DOC).** La página titulada USB del sitio de documentación contiene una plantilla de MkDocs, no una especificación del protocolo. No permite afirmar paridad USB completa. [Página USB](https://blawar.github.io/tinfoil/usb/).

## Temas y marca

IMG20 demuestra selección de tema y variaciones de composición; BIN20 incluye música y restricciones de tema comunicadas por orígenes remotos. El ZIP oficial inspeccionado trae la carpeta `themes` vacía, por lo que no proporciona un ejemplo completo del formato. Hay que inventariar recursos y configuración del tema realmente utilizado, sin publicar credenciales ni extraer contenido ajeno innecesario.

El nombre, icono hbmenu, icono de HOME si se distribuye un acceso directo, cabecera, pantalla de inicio, diálogos y recursos de NinMagicFoil deben revisarse por separado. Cambiar únicamente nombre y logotipo no satisface el requisito de reproducir navegación y funciones.

## Comparación con Sphaira 1.0.9

| Área | Base verificable en Sphaira | Trabajo pendiente para la experiencia solicitada |
|---|---|---|
| NSP/NSZ/XCI/XCZ | Instalador y múltiples orígenes. | Reproducir diálogo, opciones equivalentes, estados y comportamiento observado de Tinfoil; probar casos reales. |
| Ownfoil | Integración nativa, biblioteca por contenido, nombres y portadas, cola y reanudación. | Comprobar versión/API del servidor; añadir compatibilidad de índices clásicos si se requiere. |
| UI | Menús, rejillas, listas, diálogos y temas propios. | Construir disposición y navegación solicitadas. Los temas INI de Sphaira no prueban compatibilidad con temas de Tinfoil. |
| Nombres | Metadatos del servidor y biblioteca local; soporte de NACP comprimido incorporado antes de 1.0.9. | Definir orden de fuentes y alternativa útil para títulos sin nombre; no sustituir nombres por IDs innecesariamente. |
| Cola | 1.0.9 incorpora instalación Ownfoil en segundo plano. | Comparar cada acción y persistencia con la consola de referencia. |
| Partidas | Menús de copias/restauración y perfiles. | Comparar formatos, destinos remotos y copias compartidas; no asumir equivalencia. |
| Red y USB | HTTP, FTP, SMB y otras opciones de compilación; USB y MTP. | Verificar por separado NUT, USBFS, proveedor por proveedor y cada contrato remoto de Tinfoil. |
| Actualizador | Descarga desde GitHub, preparación temporal, validación NACP/versión y respaldo. | Canal e identidad NinMagicFoil, rutas propias, validación del paquete y recuperación probada. |
| Trucos, tickets, firmware y servicios exclusivos | No se ha acreditado equivalencia completa en esta revisión. | Inventario y diseño específicos; no presentar menús vacíos como funciones terminadas. |

Fuentes: [Sphaira 1.0.9](https://github.com/NaGaa95/sphaira/releases/tag/1.0.9), [README](https://github.com/NaGaa95/sphaira/blob/1.0.9/README.md), [API Ownfoil](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/utils/ownfoil_api.cpp), [actualizador](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/main_menu.cpp), [temas](https://github.com/NaGaa95/sphaira/tree/1.0.9/assets/romfs/themes), [partidas](https://github.com/NaGaa95/sphaira/blob/1.0.9/sphaira/source/ui/menus/save_menu.cpp).

## Dependencias que no se transfieren automáticamente

- La cuenta, los servicios premium, el catálogo remoto, las imágenes y las actualizaciones de Tinfoil pertenecen a sus servicios. Tener el binario no da control sobre esos servidores.
- Las identidades/autenticadores propios y los índices cifrados no quedan reproducidos por cambiar el nombre del cliente. Cualquier alternativa necesita un contrato compatible y acceso autorizado, sin simular capacidades no implementadas.
- Google Drive, Dropbox y 1Fichier requieren verificar APIs, autorización y vigencia de cada mecanismo; la documentación antigua no basta.
- ROMs y homebrew dependen de aplicaciones o núcleos instalados. Un listado de sistemas no incluye esos ejecutables.
- Firmware, CFW, modo de lanzamiento y hardware condicionan servicios de instalación y funciones del sistema.
- La existencia de una opción o un mensaje de error no demuestra su implementación, disponibilidad ni seguridad en la versión actual.

## Qué falta para aceptar la paridad

La siguiente etapa es un recorrido visual de Tinfoil 20 en la consola de referencia: cada apartado visible, sus submenús, ayuda contextual, opciones, teclado, controles y estados de cola. Se debe registrar también qué apartados aparecen al cambiar de ubicación o perfil. MTP no sustituye ese recorrido.

Cada función deberá tener un caso de aceptación observable y una clasificación: equivalente, variante acordada, dependiente de un servicio externo o pendiente. Los flujos de borrado, restauración, firmware y cuentas se pueden documentar sin ejecutarlos durante el reconocimiento.

Para la instalación que originó el trabajo, la evidencia actual es el fallo de lectura de metadatos/CNMT en Tinfoil y el éxito comunicado con DBI. No se ha establecido una causa técnica ni se ha atribuido el fallo a Ownfoil. Un instalador que compila y muestra el nuevo logo todavía no acredita que ese caso esté resuelto.

La comprobación final debe incluir paquetes representativos, metadatos actuales, tamaños grandes, nombres sin entrada externa, pérdida de conexión, errores de lectura, cancelación y actualización de la propia aplicación con recuperación. La afirmación «todos los juegos» no es comprobable como garantía universal; se deben publicar el alcance realmente probado y las limitaciones conocidas.
