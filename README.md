Usando el display con la lib hagl & ESP-IDF
===

Link al [Hardware Agnostic Graphics Library](https://github.com/tuupola/hagl).

* El driver funciona pero tiene unos defectos en las fonts (se ven chotas)
* Es muy simple el código y modificable
* Parece que solo es para ILI9341

No te olvides de configurar los pines del display con menuconfig!

Estos tiene que setearse, sino aparece el display girado y con los colores mal.

* Swap X & Y = true
* Mirror X = true
* BGR = true

# importante

-- Las rutinas del hagl y hagl_esp_mini están modificadas por mi.

-- La librería original de tuupola no hace una corrección de gamma que es importante para que se vean correctamente las imágenes jpeg.

-- Le agregué funciones de blit de BMP y JPG

-- Estoy en proceso de transformación, y cambiando funciones de todo tipo para mejorar rendimiento y facil uso.....

-- También me interesa hacer un buen manejo de Fonts.....

Javier 2022
