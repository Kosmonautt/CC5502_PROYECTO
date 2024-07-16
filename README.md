# CC5502_PROYECTO

**Nombres**: 
- Benjamín Contreras
- Nicolás Grandón
---

## Antes de compilar
- Para que el programa funcione correctamente:
    - Windows 10/11: 
        - WSL: Para correr el programa se instaló [WSL](https://learn.microsoft.com/en-us/windows/wsl/install), con la distribución por defecto de Ubuntu.
        - Se corrieron los siguientes comandos:
            - sudo apt update && sudo apt upgrade # actualizar el repositorio
            - sudo apt install g++ # instalar g++ se corrió
            - sudo snap install cmake --classic # instalar cmake
            - sudo apt install make # instalar make
            - sudo apt-get install libcgal-dev # instalar CGAL
            - sudo apt-get install libglfw3-dev # instalar GLFW
        - Para utilizar glad.h, se descargó del [sitio oficial](https://glad.dav1d.de/) eligiendo (cosas en la foto opengl.png), se incluyeron los archivos glad.c y glad.h en src y se en la línea 25 de glad.c, se cambió #include <glad/glad.h>
        a #include "glad.h".

## Para compilar
- Windows 10/11 (WSL Ubuntu):
    <!-- - Se utilizó "g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0", la versión de g++ que venía con la distribución.
    - Para poder construir el proyecto se utilizó CMake version 3.22.1, la versión de CMake que venía con la distribución.
    - Para hacer build, se utilizó Make 4.3, la versión de Make que venía con la distribución. -->

    - Compilación (se deben ejecutar en la ruta del proyecto):
        - rm -r build # borrar carpeta build (si es que existe)
        - mkdir build # crear carpeta build
        - cd build # entrar en carpeta build
        - cmake .. # generar makelists
        - make # generar binarios
