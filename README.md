Usando el display con la lib hagl & ESP-IDF
===

Link al [Hardware Agnostic Graphics Library](https://github.com/tuupola/hagl).

* El driver funciona pero tiene unos defectos en las fonts (se ven chotas)
* Es muy simple el código y modificable
* Parece que solo es para ILI9341

No te olvides de configurar los pines del display con menuconfig!

Estos tiene que setearse, sino aparece el display girado y con los colores mal.

* Swap X & Y = true
* BGR = true

# importante

-- Las rutinas del hagl y hagl_esp_mini están modificadas por mi.

-- Le agregué funciones de blit de BMP y JPG


Javier 2022
