# Ownfoil en NinMagicFoil 0.1.0

Esta implementación conserva el cliente de API de Sphaira y añade índices JSON y navegación de directorios de Ownfoil antiguo. Los formatos se detectan al conectar; también pueden elegirse en el formulario del servidor. Una instancia del catálogo conserva la conexión al cambiar entre juegos nuevos, actualizaciones, DLC y búsqueda.

## Servidores y autenticación

El formulario contiene nombre, URL, URL alternativa, usuario, contraseña y tipo de servidor. Admite varios perfiles, incluso para distintas cuentas del mismo servidor. Se guardan únicamente en `sdmc:/config/NinMagicFoil/ownfoil.ini`. La contraseña se oculta tanto en el formulario como en el teclado, pero el archivo de configuración de la SD la contiene en texto claro. No forma parte del código, del repositorio ni de los paquetes públicos.

Las credenciales se envían mediante autenticación HTTP Basic al catálogo, consultas de metadatos y paquetes del origen configurado. Los recursos externos no reciben automáticamente esa contraseña. Las URL no aceptan usuario/contraseña incrustados. El transporte verifica certificados HTTPS y bloquea redirecciones que cambien el origen de una solicitud autenticada; tampoco transforma un error HTTPS en un intento HTTP. La ruta antigua utiliza la misma autenticación, aunque cambia las cabeceras de identificación del cliente.

## Compatibilidad implementada

| Formato | Funcionamiento | Límites actuales |
| --- | --- | --- |
| API Ownfoil | Negociación mediante OPTIONS y consultas GraphQL; conserva las consultas y detalles del cliente Sphaira. | El servidor debe implementar esta API. No se atribuye este contrato a todas las versiones de Ownfoil. |
| Índice JSON | Lee `files`, direcciones relativas o absolutas, nombres del fragmento de URL o del campo `name`, `titledb`, tamaños de 64 bits, versiones e identificadores. | Solo índices JSON abiertos. No descifra índices privados de Tinfoil, no registra HAUTH ni sigue automáticamente listas externas. |
| Ownfoil antiguo | Lee su listado HTML autenticado y presenta carpetas que se abren bajo demanda. Envía las cabeceras del protocolo antiguo, sin User-Agent, TE ni Connection añadida por TE. | Los filtros y la búsqueda se aplican a la carpeta abierta. No se rastrean miles de carpetas del servidor para fabricar un catálogo global. |

En los índices sin metadatos se toma el nombre del archivo y se separan las etiquetas de identificador/versión. La base de títulos, cuando existe, tiene prioridad. Los paquetes sin un identificador de Switch reconocible siguen mostrando su nombre; sus categorías avanzadas no se deducen de datos inexistentes. No se inventan carátulas, fechas de lanzamiento ni nombres para un archivo que solo aporte un identificador.

Los índices JSON ofrecen juegos nuevos, actualizaciones y DLC comparando los identificadores/versiones disponibles con el inventario de la consola. Las fechas no tienen un campo uniforme en este formato: los órdenes por fecha conservan el orden de origen. Los servidores antiguos muestran sus carpetas además de los paquetes; A abre una carpeta y B permite subir mientras exista una carpeta anterior.

## Instalación y alternativa local

Desde una ficha se puede instalar directamente, descargar a la SD o descargar y después instalar desde el archivo local. Las descargas usan escritura progresiva, no reservan en memoria el tamaño del juego, y activan el soporte de archivos concatenados grandes del sistema de archivos de Switch. El archivo solo recibe su nombre final después de completar la transferencia, comprobar su firma de contenedor y verificar el tamaño y SHA-256 cuando el servidor los proporciona. Un cuerpo HTML de inicio de sesión no se acepta como NSP/NSZ/XCI/XCZ. Los archivos parciales se eliminan en caso de error o cancelación.

Los paquetes completos se conservan en `sdmc:/switch/NinMagicFoil/downloads/`, con nombres acotados y extensiones permitidas, para instalarlos localmente o abrirlos desde DBI. Esto necesita espacio para el paquete descargado, además del espacio de instalación. DBI es una aplicación separada: este módulo no incorpora su código ni supone que acepte un argumento de instalación automática. Una firma de contenedor correcta no reemplaza las verificaciones de contenido del instalador.

## Verificación realizada

- Pruebas C++ portables del parser: JSON inválido, entradas incompletas, nombres codificados, base/actualización/DLC, tamaños superiores a 4 GiB, directorios HTML y rechazo de respuestas de autenticación o páginas de login.
- El mismo parser procesó un listado real autenticado y reprodujo el número de entradas y carpetas de la referencia privada. La referencia y las credenciales permanecen fuera del repositorio.
- El 2 de noviembre de 2026 se comprobó el paquete base comunicado por el usuario mediante HEAD y dos lecturas de 16 bytes. El servidor devolvió HEAD 200, tamaño superior a 14 GB, firma PFS0 y respuestas 206 con Content-Range correcto tanto desde el byte cero como desde el desplazamiento 4.294.967.296. Se leyeron 32 bytes de contenido del paquete; no se descargó el juego completo.
- Estas sondas no demuestran que toda la instalación finalice ni identifican por sí solas la causa del error de Tinfoil. Sí descartan, en esas solicitudes concretas, una respuesta defectuosa de rangos en el límite de 4 GiB.

El lector HTML está orientado al listado de Ownfoil: no interpreta un sitio web general. Quedan por ampliar la clasificación de páginas HTML ajenas que contienen enlaces y el tratamiento de nombres con entidades HTML anidadas; pueden producir un catálogo vacío o un nombre distinto, respectivamente.

La instalación completa en consola, el comportamiento visual y las descargas grandes sobre la SD concreta requieren pruebas en el dispositivo. Las comprobaciones de red desde el ordenador no sustituyen esas pruebas.

## Referencias primarias

- [Cliente original de Sphaira](https://github.com/NaGaa95/sphaira/tree/dce51b626551ee0d3df0bbec3d335140ba6d0ccf/sphaira/source/ui/menus): base de la navegación y las operaciones de instalación de Ownfoil.
- [Identificación del cliente en Ownfoil 2.3.0](https://github.com/a1ex4/ownfoil/blob/2.3.0/app/clients/sphaira.py): explica por qué algunas instalaciones antiguas no aceptan cabeceras adicionales de identificación.
- [Cliente Sphaira de Ownfoil actual](https://github.com/a1ex4/ownfoil/blob/master/app/clients/sphaira.py): otras versiones identifican el cliente por User-Agent; el modo antiguo y el actual no son equivalentes.
- [Índices personalizados de Tinfoil](https://blawar.github.io/tinfoil/custom_index/): estructura del índice JSON compatible.
- [Cliente Tinfoil en Ownfoil](https://github.com/a1ex4/ownfoil/blob/master/app/clients/tinfoil.py): identificación específica y cifrado opcional del índice; esos servicios no se transfieren automáticamente a una aplicación distinta.
