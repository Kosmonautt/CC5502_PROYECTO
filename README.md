# CC5502_PROYECTO

**Nombres**: 
- Benjamín Contreras
- Nicolás Grandón
---

## Antes de compilar
- Para que el programa funcione correctamente:
    - Windows 10/11 (WSL Ubuntu): 
        - WSL: Para correr el programa se instaló [WSL](https://learn.microsoft.com/en-us/windows/wsl/install), con la distribución por defecto de Ubuntu.
        - Se corrieron los siguientes comandos:
            - sudo apt update && sudo apt upgrade # actualizar el repositorio
            - sudo apt install g++ # instalar g++
            - sudo snap install cmake --classic # instalar cmake
            - sudo apt install make # instalar make
            - sudo apt-get install libcgal-dev # instalar CGAL
            - sudo apt-get install libglfw3-dev # instalar GLFW
        - Para utilizar glad.h, se descargó del [sitio oficial](https://glad.dav1d.de/) eligiendo (cosas en la foto opengl.png), se incluyeron los archivos glad.c y glad.h en src y se en la línea 25 de glad.c, se cambió #include <glad/glad.h>
        a #include "glad.h".
        - Para poder leer archivos geojson se utilizó [nlohmann json](https://github.com/nlohmann/json/releases/tag/v3.11.3): Es necesario descargar el archivo "json.hpp" en "Assets" del [repositorio](https://github.com/nlohmann/json/releases/tag/v3.11.3) de github. Este archivo se debe guardar en el proyecto dentro de "/include/nlohmann" (OJO: No es una "i" o un "uno", es una "ele" minúscula).
    - Para conseguir la data en geojson de las dos comunas utilizadas como ejemplo se utilizó [Overpass Turbo](https://overpass-turbo.eu/) con las queries en data/NOMBRECOMUNA/query. Las queries en boundary.txt y schools.txt retornan la frontera de la comuna y los colegios dentro de esta respectivamente, mientras que both muestra a ambas. 

## Para compilar
- Windows 10/11 (WSL Ubuntu):
    - Compilación (se deben ejecutar en la ruta del proyecto):
        - rm -r build # borrar carpeta build (si es que existe)
        - mkdir build # crear carpeta build
        - cd build # entrar en carpeta build
        - cmake .. # generar makelists
        - make # generar binarios

## Para correr el programa
- Windows 10/11 (WSL Ubuntu):
    - Existen tres archivos generados 

## Trabajos de terceros utilizados
- Como base para poder utilizar las funciones de CGAL se utilizaron los siguientes recursos de la [página oficial](https://www.cgal.org/) de CGAL:
    - [Delaunay/voronoi en CGAL](https://doc.cgal.org/latest/Triangulation_2/Triangulation_2_2print_cropped_voronoi_8cpp-example.html)
    - [Convex Hull en CGAL](https://doc.cgal.org/latest/Convex_hull_2/Convex_hull_2_2convex_hull_indices_2_8cpp-example.html)
    - [Point_2 dentro de Polygon_2 en CGAL](https://doc.cgal.org/latest/Polygon/group__PkgPolygon2Functions.html#ga0cbb36e051264c152189a057ea385578)
    - [Intersection en CGAL](https://doc.cgal.org/latest/Kernel_23/group__intersection__linear__grp.html#gade00253914ac774cce3d2031c07d74fe)

- Para poder utilizar GLAD y GLFW utilizó como recurso los siguientes links:
    - [Learn OpenGL](https://learnopengl.com/)
    - [Setting up an OpenGL Project in Ubuntu [VSCode, GLFW, GLAD, CMake]](https://youtu.be/LxEFn-cGdE0?si=kAYliPiRIYpgTV4Q)