#include <iostream>
#include "glad.h"
#include <GLFW/glfw3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <iterator>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef K::Iso_rectangle_2 Iso_rectangle_2;
typedef K::Segment_2 Segment_2;
typedef K::Ray_2 Ray_2;
typedef K::Line_2 Line_2;
typedef CGAL::Delaunay_triangulation_2<K>  Delaunay_triangulation_2;

// struct that will store the cropped Voronoi diagram
struct Cropped_voronoi_from_delaunay{
    // this list will store the segments of the cropped Voronoi diagram
    std::list<Segment_2> m_cropped_vd;
    // this is the bounding box that will constrain the Voronoi diagram
    Iso_rectangle_2 m_bbox;

    // constructor that receives the bounding box
    Cropped_voronoi_from_delaunay(const Iso_rectangle_2& bbox):m_bbox(bbox){}

    // this template allows to use the same function for rays, lines and segments
    template <class RSL>
    // this function crops the input object to the bounding box and stores the segment
    void crop_and_extract_segment(const RSL& rsl){
        // the intersection of the rsl and the bounding box is stored in obj which supports multiple types
        CGAL::Object obj = CGAL::intersection(rsl,m_bbox);
        // if obj is a segment, it is stored in the list
        const Segment_2* s=CGAL::object_cast<Segment_2>(&obj);
        if (s) 
            m_cropped_vd.push_back(*s);
    }

    // overload the << operator to allow the cropping of rays, lines and segments
    void operator<<(const Ray_2& ray) {
        crop_and_extract_segment(ray);
    }

    void operator<<(const Line_2& line) {
        crop_and_extract_segment(line); 
    }

    void operator<<(const Segment_2& seg) {
        crop_and_extract_segment(seg); 
    }
};

// vertex shader source
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
)glsl";

// fragment shader source
const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.59, 0.11, 0.59, 1.0);
    }
)glsl";

// compile shader
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    return id;
}


// create shader program
unsigned int createShaderProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// setup buffers
void setupBuffers(unsigned int* VAO, unsigned int* VBO, const float* vertices, size_t size) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// initialize window and context
GLFWwindow* initWindowAndContext() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(640, 480, "Largest empty circle", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    return window;
}

int main(int, char**) {
    // Initialize window and context
    GLFWwindow* window = initWindowAndContext();
    // if window or context initialization failed, return -1
    if (!window) return -1;

    // Create shader program
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // this are the vertices in the figure
    float pointVertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f
    };

    // this are the edges in the figure
    float lineVertices[] = {
        -1.0f, 0.0f, 0.0f,  
        1.0f, 0.0f, 0.0f, 
        // -0.5f, -0.5f, 0.0f, 
        // 0.5f, -0.5f, 0.0f,  
        // 0.5f, -0.5f, 0.0f,  
        // 0.0f, 0.5f, 0.0f    
    };

    // buffer for the vertices
    unsigned int pointVAO, pointVBO;
    setupBuffers(&pointVAO, &pointVBO, pointVertices, sizeof(pointVertices));

    // buffer for the edges
    unsigned int lineVAO, lineVBO;
    setupBuffers(&lineVAO, &lineVBO, lineVertices, sizeof(lineVertices));


    // the size of the points is set to 10.0f
    glPointSize(10.0f);

    // the background color is set to purple
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        // this allows to close the window by pressing the ESC key
        glfwPollEvents();
        // this allows to clear the window with a color
        glClear(GL_COLOR_BUFFER_BIT);

        // this tells the GPU to use the shader program
        glUseProgram(shaderProgram);


        // the vertices are drawn
        glBindVertexArray(pointVAO);
        glDrawArrays(GL_POINTS, 0, 3);

        // the edges are drawn
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 2);

        glfwSwapBuffers(window);
    }

    // clean up
    glDeleteVertexArrays(1, &pointVAO);
    glDeleteBuffers(1, &pointVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}