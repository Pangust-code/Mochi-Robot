# GIFs disponibles para Mochi Robot

Colección de 99 animaciones listas para convertir al formato `.bin` que usa el robot.
Cada GIF debe ser procesado con la herramienta en [herramientas/convertidor_gifs/](../../herramientas/convertidor_gifs/) antes de cargarse al ESP32.

---

## Catálogo

| Archivo | Descripción |
|---------|-------------|
| `0.gif` | Animación por defecto |
| `angry.gif` / `angry2.gif` / `angry3.gif` | Expresiones de enojo |
| `aot.gif` | Attack on Titan |
| `bee.gif` | Abeja |
| `bmo.gif` | BMO (Hora de Aventura) |
| `chong_mat.gif` | Mareado |
| `cry.gif` / `cry2.gif` | Llorando |
| `cuoi.gif` / `cuoi_khinh_bi.gif` | Sonriendo / risa burlona |
| `dasai.gif` / `dasai2.gif` | Dasai (mascota del robot) |
| `demon_slayer.gif` | Demon Slayer |
| `den_giao_thong.gif` / `den_pha.gif` / `den_pha_ex.gif` | Luces / semáforo |
| `devil.gif` / `devil_2.gif` / `devil_eyes.gif` | Demonio |
| `distracted.gif` | Distracted boyfriend meme |
| `egg.gif` | Huevo |
| `gian_du.gif` | Expresión traviesa |
| `glitch.gif` | Efecto glitch |
| `gundam.gif` | Gundam |
| `hand_some.gif` | Guapo |
| `happy_2.gif` | Feliz |
| `hat_xi.gif` / `hat_xi_2.gif` | Estornudando |
| `hehe.gif` | Risita |
| `i-guess-bro-what-are-you-talking-about.gif` | Meme "I guess bro" |
| `intro.gif` | Pantalla de intro |
| `keep_it_up.gif` | Ánimo |
| `khoc.gif` | Llorando fuerte |
| `leu_leu.gif` | Burlándose |
| `love.gif` / `love2.gif` | Amor |
| `mochi.gif` / `mochidoo.gif` | Mascota Mochi |
| `music.gif` | Musical |
| `neon.gif` | Efecto neón |
| `ngac_nhien.gif` | Sorprendido |
| `ngap.gif` | Bostezando |
| `nheo_mat.gif` | Guiñando |
| `nhin_ben_phai.gif` / `nhin_ben_trai.gif` / `nhin_xuong.gif` | Mirando a los lados / abajo |
| `noname.gif` | Sin nombre |
| `police.gif` | Policía |
| `pong.gif` | Pong |
| `rain.gif` | Lluvia |
| `rainbow.gif` | Arcoíris |
| `sakura.gif` | Sakura |
| `samurai.gif` | Samurái |
| `scared.gif` | Asustado |
| `sleepy.gif` | Somnoliento |
| `smile.gif` | Sonrisa |
| `smirk.gif` | Sonrisa pícara |
| `speed.gif` / `speed_ex.gif` | Velocidad |
| `squint.gif` | Ojos entrecerrados |
| `star.gif` | Estrella |
| `sung_nuoc.gif` / `sung_nuoc_2.gif` | Burbuja de agua |
| `sushi.gif` | Sushi |
| `thong_long.gif` | Dragón |
| `thu_nho.gif` | Encogerse |
| `toc_do.gif` | Velocidad extrema |
| `turbo.gif` | Turbo |
| `up_size_down.gif` | Crecer y encoger |
| `UwU.gif` | UwU |
| `wheels.gif` | Ruedas girando |
| `xi_khoi.gif` / `xi_lua.gif` | Humo / llama |
| `yakura.gif` | Explosión |

---

## Cómo agregar un GIF personalizado

1. Coloca tu GIF en esta carpeta (`recursos/gifs/`)
2. Asegúrate de que sea **en blanco y negro** o de bajo contraste (la OLED es monocromática)
3. Procésalo con el convertidor: ver [herramientas/convertidor_gifs/](../../herramientas/convertidor_gifs/)
4. Copia el `.bin` resultante a `03_firmware/mochi_unified_5/data/`
5. Sube la carpeta `data/` al ESP32 con el plugin LittleFS
