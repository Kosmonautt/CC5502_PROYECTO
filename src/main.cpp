#include <iostream>
#include "glad.h"
#include <GLFW/glfw3.h>

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
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
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

    float vertices[] = {
        0.0f, 0.5f, 0.0f,  // Point 1
        -0.5f, -0.5f, 0.0f, // Point 2 (Line 1: Point 1 -> Point 2)

        -0.5f, -0.5f, 0.0f, // Point 2
        0.5f, -0.5f, 0.0f,  // Point 3 (Line 2: Point 2 -> Point 3)

        0.5f, -0.5f, 0.0f,  // Point 3
        0.0f, 0.5f, 0.0f    // Point 1 (Line 3: Point 3 -> Point 1)
    };

    // Setup buffers
    unsigned int VAO, VBO;
    setupBuffers(&VAO, &VBO, vertices, sizeof(vertices));

    // the size of the points is set to 10.0f
    glPointSize(10.0f);

    // the background color is set to purple
    glClearColor(0.5f, 0.0f, 0.5f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        // this allows to close the window by pressing the ESC key
        glfwPollEvents();
        // this allows to clear the window with a color
        glClear(GL_COLOR_BUFFER_BIT);

        // this tells the GPU to use the shader program
        glUseProgram(shaderProgram);
        // this tells the GPU to use the VAO
        glBindVertexArray(VAO);

        // this are the points to draw
        glDrawArrays(GL_POINTS, 0, 6);

        // this are the lines to draw
        glDrawArrays(GL_LINES, 0, 6);

        glfwSwapBuffers(window);
    }

    // clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}