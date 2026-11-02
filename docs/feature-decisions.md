# Decisiones sobre las funciones

El propietario autorizó retirar funciones obsoletas o sin utilidad para NinMagicFoil, con explicación. Eso **no** convierte una función todavía no implementada en obsoleta. Esta lista diferencia ambos casos y evita declarar que existe paridad completa con Tinfoil.

## Funciones que no se trasladan por depender de la aplicación original

| Función de Tinfoil | Decisión y motivo |
|---|---|
| Cuenta y actualizador de Tinfoil | No se trasladan. Administran Tinfoil y su servicio, no las versiones de NinMagicFoil. Los sustituye un canal propio de GitHub. No se afirma que su servicio haya desaparecido. |
| Aceptación de términos de Tinfoil | No se muestra un ajuste de aceptación de otro producto. NinMagicFoil conserva y distribuye su licencia GPL y las licencias de terceros. |
| Lista fija de payloads y textos promocionales de Tinfoil | No se copia esa lista. Un nombre fijo no demuestra que exista un payload compatible en la SD. La carga de payloads requiere detección y validación propias antes de ofrecerse. Esto no declara obsoletos todos los payloads ni Hekate. |
| Precarga obligatoria de toda la base de datos/carátulas | Se utiliza consulta paginada y caché de las imágenes solicitadas. No hace falta descargar el catálogo completo para mostrar una página. La precarga manual completa no está implementada en 0.1.0. |

## Funciones agrupadas o sustituidas

| Función | Ubicación o sustitución |
|---|---|
| Juegos nuevos, actualizaciones y DLC | Categorías Ownfoil del menú lateral. |
| XCI | Vista XCI/XCZ del explorador y el instalador común. No equivale aún a una tabla remota XCI idéntica a la de Tinfoil. |
| Copias de partidas y selección de usuario | Gestor de partidas de Sphaira. No se considera obsoleta. |
| Explorador local, USB y fuentes de red | Explorador, montajes y sección USB y red. |
| Consola | Visor local del registro de NinMagicFoil. |
| Temas, idioma, música y opciones de instalación | Opciones de NinMagicFoil/Sphaira. |
| Elegir pestañas izquierda/centro/derecha de Sphaira | Se sustituye por la barra lateral. Esos ajustes ya no controlarían la nueva navegación y no se muestran como opciones operativas. |
| Sustituir hbmenu, función de Sphaira | Deshabilitada en NinMagicFoil. Esta aplicación tiene su propia ruta y no necesita sustituir el menú homebrew para funcionar. |

## Pendientes; no retiradas por obsolescencia

- Paridad de todos los diálogos, columnas, atajos y modos de presentación de Tinfoil. Algunas herramientas conservan las pantallas de Sphaira.
- Lista independiente de tickets y herramientas de titlekeys. El instalador puede gestionar el contenido necesario sin que exista la pantalla equivalente.
- Catálogos separados para NES, SNES, N64, NDS, Game Boy, GBA, Sega y PlayStation. El explorador de Sphaira conserva asociaciones con emuladores instalados; no se ha replicado cada catálogo de Tinfoil.
- Gestión específica de actualizaciones del sistema y selección/lanzamiento de payloads en una pantalla dedicada.
- Funciones de cuenta de Nintendo, modo parental equivalente y servicios de partidas compartidas. No se presentan como funciones inútiles.
- Integraciones directas Google Drive y 1Fichier de Tinfoil; se conserva la posibilidad de que el servidor Ownfoil gestione sus fuentes, pero eso no equivale a implementar sus clientes en la consola.
- Prueba del archivo base de 14 GB, juegos modernos y todos los caminos de instalación en hardware real.

Si una función está pendiente, ocultar una entrada vacía evita simular una función operativa; no significa que el requisito se dé por terminado.
