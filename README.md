# CC5502_PROYECTO

**Nombres**: 
- Benjamín Contreras
- Nicolás Grandón
---

## Antes de compilar
- Para que el programa funcione correctamente:
    - Windows 10: 
        - WSL: Para correr el programa se instaló [WSL](https://learn.microsoft.com/en-us/windows/wsl/install), con la distribución por defecto de Ubuntu, para tener el subsitema acuatlizado se corrió el comando: 
            - sudo apt update && sudo apt upgrade
        - Para instalar g++ se corrió el comando:
            - sudo apt install g++
        - Para instalar cmake se corrió el comando:
            - sudo snap install cmake --classic
        - Para instalar make se corrió el comando:
            - sudo apt install make
        - Para instalar CGAL se corrió el siguiente comando:
            - sudo apt-get install libcgal-dev
        - Para instalar GLFW se corrió el siguiente comando:
            - sudo apt-get install libglfw3-dev
        - Para utilizar glad.h, se descargó del [sitio oficial](https://glad.dav1d.de/) eligiendo (cosas en la foto opengl.png), se incluyeron los archivos glad.c y glad.h en src y se en la línea 25 de glad.c, se cambió #include <glad/glad.h>
        a #include "glad.h".
