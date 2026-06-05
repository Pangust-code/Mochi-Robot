# Recursos — Animaciones para Mochi

Esta carpeta contiene materiales listos para usar en la personalización visual del robot.

| Carpeta | Contenido |
|---------|-----------|
| [gifs/](gifs/) | Animaciones GIF listas para convertir y cargar en Mochi |

---

## Fase 4 — Elige la personalidad de Mochi

Mochi puede mostrar cualquier animación compatible con su pantalla OLED de 128×64 píxeles. Tu tarea en esta fase es elegir GIFs que expresen emociones o situaciones específicas y convertirlos al formato del robot.

### Emociones sugeridas para buscar

| Emoción / Situación | Para cuándo |
|--------------------|-------------|
| Feliz / celebrando | Reacción a un logro o saludo |
| Enojado / frustrado | Reacción a ruido fuerte o interrupción |
| Sorprendido | Sobresalto por sonido inesperado |
| Durmiendo / aburrido | Estado de reposo sin actividad |
| Bailando / entusiasmado | Reacción táctil especial |

Puedes elegir GIFs de `gifs/` o traer los tuyos propios.

---

## Criterios de selección

Un GIF se verá bien en Mochi si cumple lo siguiente:

### Buenas características

- **Alto contraste:** fondo oscuro con figura clara, o viceversa. La OLED solo muestra blanco o negro, sin grises.
- **Formas grandes:** figuras simples y reconocibles a baja resolución.
- **Pocas frames:** 10–30 frames dan animaciones fluidas sin ocupar demasiado espacio en LittleFS.
- **Blanco y negro preferentemente:** los GIFs en color se convierten automáticamente, pero los resultados son mejores si ya tienen buen contraste.

### Características a evitar

- **Fotografías o gradientes:** la conversión a blanco/negro los hace irreconocibles.
- **Texto pequeño:** ilegible a 128 px de ancho.
- **GIFs muy largos (más de 100 frames):** pueden no caber en LittleFS o hacer que el Modo 1 tarde mucho en avanzar.
- **Colores similares en contraste bajo:** por ejemplo, azul oscuro sobre negro se verá todo negro.

### Comparación rápida

| Tipo de GIF | Resultado en OLED |
|-------------|------------------|
| Caricatura blanco/negro, contornos gruesos | Excelente |
| Pixel art de 8 bits | Excelente |
| Animación de icono simple | Muy bueno |
| Emoji animado sobre fondo blanco | Bueno |
| Meme con foto de persona | Deficiente |
| Gradiente de colores | No recomendado |

---

## GIFs disponibles en esta carpeta

La carpeta `gifs/` contiene animaciones listas para convertir. Están organizadas en categorías:

- Expresiones de personajes (emociones variadas)
- Iconos y objetos animados
- Personajes de videojuegos y cultura pop

> Los archivos GIF son binarios y pueden ser grandes (15 KB – varios MB).
> Se recomienda usar [Git LFS](https://git-lfs.com) si los rastrearás en git,
> o descargarlos por separado y agregarlos a esta carpeta localmente.

---

## Cómo usar un GIF de esta carpeta

1. Copia el archivo `.gif` que quieres usar a `herramientas/convertidor_gifs/`.
2. Sigue el pipeline completo en [herramientas/README.md](../herramientas/README.md).
3. Anota el nombre del `.bin` generado — lo necesitas en la Fase 5.

---

## Dónde encontrar más GIFs

Si los GIFs disponibles no son lo que buscas, puedes conseguir más en:

- [GIPHY](https://giphy.com) — búsqueda por emoción o personaje
- [Tenor](https://tenor.com) — alternativa a GIPHY
- [LottieFiles](https://lottiefiles.com) — animaciones vectoriales (exportar como GIF)
- Pixel art propio creado con [Piskel](https://www.piskelapp.com) (gratuito, en el navegador)

Busca términos como: `pixel art angry`, `cute robot emotion`, `black white animation`, `8bit expression`.

---

**Siguiente paso:** [herramientas/README.md](../herramientas/README.md) — convertir el GIF elegido al formato `.bin`
