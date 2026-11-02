# NinMagicFoil

Cliente para Nintendo Switch basado en **Sphaira 1.0.9**, con la marca NinMagic y una interfaz inspirada en la navegación lateral de Tinfoil. Proyecto independiente, publicado bajo GPL-3.0.

## Estado

Versión inicial **0.1.0** en desarrollo. El código ya se compila para Switch; la validación de instalación en una consola real está pendiente. Esta versión no reproduce todavía todas las funciones ni todas las pantallas de Tinfoil. No se afirma compatibilidad universal con todos los juegos ni que esté resuelto el caso concreto del NSP de 14 GB.

## Qué incluye

- Menú lateral, cabecera con espacio libre y reloj, tema gris/turquesa y logo NinMagic.
- Juegos instalados, carátulas y metadatos; explorador de archivos y paquetes NSP, NSZ, XCI y XCZ mediante el instalador de Sphaira.
- Ownfoil: varios perfiles con URL, URL alternativa, usuario y contraseña; API nativa, índices JSON y listado HTML clásico con carpetas, búsqueda y nombres de catálogo.
- Autenticación en el catálogo y en los archivos protegidos. Las credenciales permanecen en la configuración local de la consola y no se distribuyen con la aplicación.
- Cola de instalaciones, partidas guardadas, cartuchos, homebrew, transferencias USB/MTP/FTP y montajes de red.
- Actualizador propio desde las publicaciones de `ANE-JV/ninmagicfoil`, con comprobación de integridad y copia anterior.

También incluye descarga autenticada a `/switch/NinMagicFoil/downloads/`, instalación del archivo descargado y acceso a **USB y red → Abrir DBI**. DBI es una aplicación externa: no se incorpora su motor; el usuario selecciona el archivo local dentro de DBI. Consulta [soporte Ownfoil y comprobaciones realizadas](docs/ownfoil-support.md).

Se ha comprobado desde el PC el acceso autenticado al servidor real de referencia y las respuestas de un paquete de 14.049.497.676 bytes: cabecera NSP y rango por encima de 4 GiB correctos. Solo se leyeron 32 bytes del paquete. Esto no es una prueba de instalación en la consola.

## Instalación

Cuando haya una publicación disponible, extrae `ninmagicfoil.zip` en la raíz de la microSD. La ruta del ejecutable debe quedar así:

```
/switch/NinMagicFoil/NinMagicFoil.nro
```

Inícialo desde el menú homebrew. La configuración se guarda en `/config/NinMagicFoil/`. No reemplaza `hbmenu.nro`. Para configurar una tienda, abre **Servidores Ownfoil**, añade su URL y los datos de acceso, y guarda el perfil. No incluyas esos datos al publicar informes de errores.

Para actualizar desde la aplicación: **Opciones → Actualizar NinMagicFoil**. Hace falta una publicación estable con el archivo exacto `ninmagicfoil.zip` y el resumen SHA-256 que proporciona GitHub. Si no existe una publicación o no se puede verificar, no reemplaza el ejecutable.

## Compilación y comprobaciones

Se incluyen scripts para devkitPro en Windows y Linux, un SDK libnx fijado y un flujo de GitHub Actions. La verificación comprueba la estructura NRO, nombre, versión, icono y contenido del paquete; no sustituye una prueba en hardware.

- Windows: `tools/build-ninmagicfoil.ps1`.
- Linux/devkitPro: `tools/build-ninmagicfoil.sh`.
- Paquete: `tools/package-ninmagicfoil.py`.
- Verificador: `tools/verify-ninmagicfoil.py`.

## Documentación

- [Ownfoil, autenticación y modo alternativo](docs/ownfoil-support.md).
- [Decisiones de funciones: retiradas, agrupadas y pendientes](docs/feature-decisions.md).
- [Referencia de las pantallas observadas](docs/current-ui-reference.md).
- [Inventario de Tinfoil](docs/tinfoil-inventory.md).
- [Estudio de integración y compatibilidad](docs/sphaira-integration.md).
- [Pruebas pendientes en consola](docs/hardware-validation.md).
- [Procedencia y licencias](docs/provenance.md).

## Créditos

NinMagicFoil conserva el código y los avisos de Sphaira y de sus dependencias. Base fijada: [NaGaa95/sphaira, versión 1.0.9](https://github.com/NaGaa95/sphaira/tree/1.0.9), commit `dce51b626551ee0d3df0bbec3d335140ba6d0ccf`. El logotipo NinMagic fue proporcionado por el propietario del proyecto. Consulta [LICENSE](LICENSE).
