# Referencia de la interfaz actual

Observación del vídeo aportado por el usuario el 2 de noviembre de 2026. Duración: 42,91 segundos; imagen original: 474 × 850, 30 fps. Se examinaron fotogramas cada medio segundo y fotogramas adicionales en las transiciones rápidas. Los fotogramas y recortes de análisis permanecen en `.local/reference-video/`; no deben formar parte de una publicación.

Este documento contiene solo observaciones de interfaz, sin nombres de la biblioteca, credenciales, direcciones de servidores ni fotografías. Los tiempos son aproximados. Los valores indicados son los observados en esa consola, no valores predeterminados recomendados para NinMagicFoil. El vídeo muestra navegación; no valida la ejecución de todas las acciones.

## Distribución y aspecto

- Tema oscuro: fondo gris, texto principal blanco, texto secundario gris claro y acento turquesa. El elemento con foco tiene un marco rectangular fino turquesa, con el texto destacado en ese color.
- Cabecera persistente: marca a la izquierda, guion y nombre de la sección; a la derecha aparecen memoria del sistema, tarjeta microSD, dos barras de espacio, reloj e indicadores. La cabecera mantiene algunos títulos en inglés aunque la barra lateral está traducida.
- Barra lateral de aproximadamente un tercio del ancho, con icono y rótulo por entrada. Una línea vertical la separa del contenido. Se ven aproximadamente siete entradas a la vez, y la lista se desplaza conservando la selección.
- Contenido principal de aproximadamente dos tercios del ancho. Las opciones ocupan filas de altura variable: nombre a la izquierda, valor alineado a la derecha y explicación debajo cuando existe. La lista desplaza su contenido al mover el foco.
- Pie persistente con iconos de vista/perfil a la izquierda y acciones contextuales. Con foco en las opciones, el pie se simplifica a la acción de volver.
- La biblioteca instalada se presenta en una rejilla de seis columnas de iconos cuadrados; en esta vista caben aproximadamente cuatro filas. Los catálogos nuevos y XCI se muestran como tablas densas.
- Las tablas visibles incluyen `Nombre`, `Tipo`, `Versión`, `Región`, `Tamaño` y una columna de fecha cuyo rótulo parece `Publicación`. El último rótulo tiene menor legibilidad que los demás. No se transcriben las filas de la biblioteca.
- La consola de registro utiliza texto pequeño alineado a la izquierda. Ayuda presenta encabezados y párrafos en una lista desplazable.

Las medidas son relativas porque el vídeo filma la pantalla en perspectiva. Para un lienzo de 1280 × 720, una primera aproximación de implementación es un divisor lateral alrededor de x=400, cabecera de 80–90 px y pie desde y≈650. Es una aproximación visual, no una medición directa de píxeles de la aplicación. Los separadores, márgenes y tipografía se deben ajustar en la previsualización.

## Menús visibles y orden

El recorrido de 29 a 42 segundos permite identificar esta secuencia de la barra lateral. Se conserva la escritura visible, aunque algunas traducciones sean irregulares.

| Orden | Rótulo observado | Encabezado o estado observado |
|---|---|---|
| 1 | Instalados | `Installed`; rejilla de iconos. |
| 2 | Explorador | `File Browser`; lista de ubicaciones. |
| 3 | Juegos Nuevos | `New Games`; tabla de títulos. |
| 4 | Updates Nuevos | `New Updates`; tabla de actualizaciones. |
| 5 | Actualizaciones del sist… | El rótulo largo queda cortado; no se abre claramente durante el recorrido. |
| 6 | XCI | `XCI`; tabla. |
| 7 | payloads | Entrada visible; no se valida su acción. |
| 8 | NES | `NES`; tabla. |
| 9 | SNES | `SNES`; tabla. |
| 10 | N64 | Entrada visible durante el desplazamiento. |
| 11 | NDS | `NDS`; tabla. |
| 12 | Gameboy | `Gameboy`; tabla. |
| 13 | GBA | Entrada visible durante el desplazamiento. |
| 14 | Sega Master System | Encabezado visible al recorrer emuladores. |
| 15 | Sega Genesis | Encabezado visible; tabla. |
| 16 | Sega CD | Entrada visible durante el desplazamiento. |
| 17 | Game Gear | Encabezado visible; tabla. |
| 18 | Play Station | Encabezado visible; tabla. |
| 19 | PlayStation Portable | Entrada visible al inicio y al final de la lista de sistemas. |
| 20 | Cola | `Queue`; panel vacío en los momentos mostrados. |
| 21 | Tickets | Entrada visible. |
| 22 | Extraer Titlekeys | `Title Keys`; no se transcribe contenido. |
| 23 | Consola | `Console`; registro de texto. |
| 24 | Opciones | `Options`; lista de ajustes descrita abajo. |
| 25 | Ayuda | `Help`; lista de artículos. |

Este vídeo no demuestra que esos sean todos los apartados que puede crear Tinfoil. Por ejemplo, `New DLC`, `Recommended`, `Shared Saves` y `Homebrew Store` figuraban en otras evidencias del estudio, pero no se reconocen como pantallas activas en este recorrido. No deben confundirse las ventanas guardadas con las pantallas aquí observadas.

## Opciones: transcripción en orden

`SI` y `NO` conservan la grafía observada. «Sin valor visible» no equivale a comprobar que una configuración está vacía. Los campos sensibles se identifican únicamente por su nombre.

| N.º | Rótulo visible | Valor o presentación observada | Tiempo de referencia |
|---|---|---|---|
| 1 | Tema | `Auto` | 0,0 s; 6,5–7,0 s |
| 2 | Región | `US` | 0,0–1,0 s; 6,0–7,0 s |
| 3 | Idioma | `es`; explicación debajo sobre reinicio | 0,0–1,0 s; 7,0 s |
| 4 | Modo parental | `NO` | 0,0–1,0 s; 5,5–7,5 s |
| 5 | Vincular Cuentas a Nintendo Online | Acción, sin valor a la derecha | 7,5–7,7 s |
| 6 | Almacenar todas las partidas de este usuario en un directorio | Nombre largo; extremo derecho superpuesto o cortado. El valor no se puede transcribir con fiabilidad. | 5,0–5,2 s; 7,7–8,0 s |
| 7 | Guardar una copia del título instalado | `NO` | 4,5–4,8 s |
| 8 | Mostrar sólo títulos con archivos | `NO` | 4,5–4,8 s |
| 9 | Nivel de registro | `Warning` | 2,0–2,5 s; 4,5–4,8 s |
| 10 | Debes de aceptar los términos y condiciones | `De acuerdo` | 2,0–2,5 s; 4,5–4,8 s |
| 11 | Habilitar Música | `SI` | 2,5–3,5 s; 8,5–9,0 s |
| 12 | Hilos de descarga | `1` | 3,5 s; 9,0 s |
| 13 | KEY de Google Api | Campo de credencial; no se publica valor | 3,0–3,5 s; 9,5–9,95 s |
| 14 | KEY de 1Fichier API | Campo de credencial; no se publica valor | 9,5–9,95 s |
| — | Instalar | Encabezado de grupo | 9,95–10,35 s |
| 15 | Instalar código sin firmar | `SI` | 9,95–10,35 s |
| 16 | Instalar actualizaciones desde XCI | `NO` | 10,15–10,65 s |
| 17 | Instalar DLCs Desde XCI | `NO` | 10,35–10,65 s |
| 18 | Ignorar la versión del firmware | `SI` | 10,35–11,0 s |
| 19 | Instalar Directo | `SI` | 10,65–11,5 s |
| 20 | Extraer los Iconos de NSP | `SI` | 11,5–12,0 s |
| 21 | Descargar iconos | `SI` | 12,0–12,5 s |
| 22 | Precargar Imágenes DLC | `NO` | 12,0–12,5 s |
| 23 | Precargar base de datos completa | `NO` | 12,0–13,5 s |
| 24 | Precargar todas las imágenes | Acción, sin valor a la derecha | 12,5–13,5 s |
| 25 | Precargar iconos | Acción, sin valor a la derecha | 13,5–14,0 s |
| 26 | Tiempo de espera de conexión agotado | `15`; no se ve unidad junto al número | 13,5–14,0 s |
| 27 | Respaldo de partidas guardadas | Ruta local de destino; valor omitido en el documento público | 14,0–14,5 s |
| 28 | eShop Dump | Ruta local de destino; valor omitido en el documento público | 14,0–14,5 s |
| — | Tinfoil.io Account | Encabezado de grupo | 15,0–15,5 s |
| 29 | Nombre de usuario | Campo de cuenta; no se publica valor | 15,0 s |
| 30 | Contraseña | Campo de cuenta; no se publica valor | 15,0–16,0 s |
| 31 | Habilitar MTP | `SI` | 16,0–18,0 s |
| 32 | Enable NUT | `SI`; el rótulo permanece en inglés | 16,0–18,0 s |
| 33 | Desactivar Instalación de Cartucho | `NO` | 16,5–18,0 s |

Después de la última opción se ve brevemente un rectángulo de foco sin texto legible. No hay evidencia suficiente para atribuirle una acción adicional. Tampoco aparece una pantalla de actualización de la propia aplicación en este vídeo; el actualizador solicitado para NinMagicFoil deberá verificarse mediante sus propios casos de aceptación.

## Ayuda visible

Entre 18,5 y 28,5 segundos se recorre la ayuda. Estos son los encabezados reconocibles, sin copiar los párrafos:

1. Instalación de juegos a través de USB.
2. Instalación de juegos a través de la Red.
3. Instalación de juegos a través de tarjeta microSD.
4. `Sección 'Nuevos Juegos No está?'` (traducción visible irregular).
5. Un artículo sobre el error de datos dañados al ejecutar XCI.
6. Google Drive.
7. Dropbox.
8. Juegos de Guarda.
9. Trampas de juego.
10. El lanzamiento de ROM emulador.
11. Retroarch Cores / ROM Directories.
12. Exportar Juego Instalado a NSP.
13. Instalar firmware.

La ayuda está dentro del panel principal y usa foco turquesa sobre el encabezado seleccionado. Al desplazarse conserva el marco global, el menú lateral y el pie. Ver el artículo de firmware, cuentas o trucos no demuestra que se haya ejecutado esa función.

## Controles y comportamiento de navegación

- Al mover el foco por la barra lateral cambia el panel de contenido y el título de cabecera inmediatamente; el vídeo no muestra una transición a pantalla completa para cada categoría.
- En las opciones, la selección recorre filas y desplaza la lista. El rótulo lateral `Opciones` sigue coloreado mientras el marco de foco está en el panel derecho.
- Al volver desde el panel de opciones al lateral, reaparece el conjunto general de acciones del pie. El listado conserva la posición cerca de su final al regresar desde otras categorías.
- Con foco general se leen `Búsqueda`, `Usuario`, `Próximo usuario`, `Refrescar`, `Salir`, `Aceptar`. Con foco en opciones se lee `Atrás` a la derecha.
- Los glifos de botones están presentes, pero su interior no tiene resolución suficiente para transcribir todas las letras sin apoyarse en otras referencias. El estudio histórico asocia búsqueda con +, usuarios con L/R, refrescar con X y salir/aceptar con B/A; esa asociación no se presenta aquí como lectura nueva del vídeo.
- Se observa tanto selección de categoría como selección de una fila del panel derecho. La implementación debe tratar ambos focos de forma distinta y conservar el estado de cada categoría.
- Hay listas con varias filas coloreadas durante el paso rápido por sistemas. La grabación no permite concluir si representan selección múltiple, estado de los elementos o artefactos del movimiento; no se deriva de ello un comportamiento de selección múltiple.

## Consecuencias para la primera implementación

La referencia actual permite construir el marco oscuro, la barra lateral desplazable, dos focos de navegación, vistas de tabla/rejilla, opciones con explicación y valores, ayuda y pie contextual. Deben existir acciones reales detrás de las entradas que se presenten como funcionales. Copiar los rótulos de cuentas, firmware, claves o proveedores externos no implementa sus servicios.

Los valores personales observados no deben convertirse en valores predeterminados: por ejemplo, permitir código sin firmar o ignorar comprobaciones de firmware es una configuración de esa instalación, no un requisito de comportamiento inicial. El nombre y el logo solicitados deben sustituirse en el marco y el icono de NinMagicFoil, manteniendo una presentación coherente en sus propias pantallas.

Quedan fuera de lo demostrado por este vídeo los diálogos de instalación, detalle del juego, filtros, edición de ubicación, teclado, ejecución de backups, restauración, operaciones sobre tickets y fallo/recuperación de la cola. Las referencias adicionales del proyecto pueden documentarlos; esta transcripción no rellena los huecos con suposiciones ni requiere nuevas fotografías.
