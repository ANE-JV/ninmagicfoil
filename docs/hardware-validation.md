# Validación en consola

Estado de ejecución en consola: **pendiente**. Compilar un NRO, validar sus cabeceras y superar las pruebas de parsers no valida una instalación real ni el estado de la microSD.

## Comprobación previa desde PC

El servidor autenticado de referencia devuelve el catálogo clásico. En el paquete de 14.049.497.676 bytes se verificaron HEAD 200 y dos rangos de 16 bytes: offset 0 con magia PFS0, y offset 4.294.967.296 con respuesta 206 y Content-Range correctos. No se descargó ni instaló el paquete completo. No se ha encontrado un fallo básico de rangos mayores de 4 GiB en ese servidor.

## Interfaz

- Arranque desde `/switch/NinMagicFoil/NinMagicFoil.nro`; nombre e icono NinMagic en hbmenu y dentro de la aplicación.
- Movimiento entre menú lateral y contenido con mando y pantalla táctil; vuelta con B/ZL sin perder la conexión Ownfoil.
- Biblioteca con nombres y carátulas; catálogo vacío, metadatos ausentes, nombres largos y caracteres multilingües.
- Navegación por las herramientas que conservan pantallas de Sphaira; retorno correcto al menú.

## Ownfoil autenticado

- Crear dos perfiles distintos; URL, usuario y contraseña se conservan tras reiniciar.
- Contraseña oculta en el formulario; respuestas 401/403 identificables y reintento tras corregirla.
- Catálogo, carátulas, consultas de tamaño/rango y descarga usan las credenciales del servidor correspondiente.
- No se envían credenciales del servidor a orígenes externos ni se publican en registros.
- HTTPS válido, certificado no confiable, redirección del servidor y caída de conexión.
- API nativa y JSON clásico por separado; no dar un formato por validado por probar el otro.

## Caso del archivo de 14 GB

1. Registrar versión de NinMagicFoil, sistema y entorno; evitar incluir datos de acceso en el informe.
2. Confirmar tamaño de 64 bits y respuesta de rangos del servidor sin descargar todo el archivo para una simple consulta.
3. Instalar directamente desde Ownfoil. Conservar el código exacto del error si falla.
4. Descargar primero a SD mediante la misma autenticación; comprobar tamaño/integridad disponibles y espacio temporal.
5. Instalar el archivo local con NinMagicFoil.
6. Si es necesario, abrir DBI y seleccionar ese mismo archivo local; registrar si su resultado difiere.

No se debe describir el problema de Tinfoil como resuelto hasta probar el archivo exacto. Los mensajes de metadatos/CNMT no identifican por sí solos si el origen es el contenedor, la lectura remota, el entorno o el instalador.

## Otros formatos y errores

Probar NSP/NSZ/XCI/XCZ pequeños y grandes; base, actualización y DLC; microSD FAT32; instalación cancelada; almacenamiento insuficiente; servidor lento; transferencia incompleta; cola con varios elementos y salida durante una instalación.

## Actualizador

Probar una publicación posterior con nombre, versión, tamaño y SHA-256 correctos; publicación sin activo; integridad errónea; ZIP ajeno; NRO de otro producto; descarga interrumpida; error de escritura y recuperación de la copia anterior. No se ha validado resistencia frente a un corte de energía en el momento del reemplazo.
