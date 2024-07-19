#include <utility> 
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "glad.h"
#include <GLFW/glfw3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/Convex_hull_traits_adapter_2.h>
#include <CGAL/property_map.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/squared_distance_2.h>
#include <iterator>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef K::Iso_rectangle_2 Iso_rectangle_2;
typedef K::Segment_2 Segment_2;
typedef K::Ray_2 Ray_2;
typedef K::Line_2 Line_2;
typedef CGAL::Delaunay_triangulation_2<K>  Delaunay_triangulation_2;
typedef CGAL::Convex_hull_traits_adapter_2<K, CGAL::Pointer_property_map<Point_2>::type > Convex_hull_traits_2;
typedef CGAL::Polygon_2<K> Polygon_2;

// the color of the input points (red)
float inputPointsColor[3] = {1.0f, 0.0f, 0.0f};
// the color fo the voronoi edges (green)
float voronoiEdgesColor[3] = {0.0f, 1.0f, 0.0f};
// the color of the convex hull (blue)
float convexHullColor[3] = {0.0f, 0.0f, 1.0f};
// the color of the candidate points (yellow)
float candidatePointsColor[3] = {1.0f, 1.0f, 0.0f};
// the color of the largest empty circle (white)
float largestEmptyCircleColor[3] = {1.0f, 1.0f, 1.0f};

// show voronoi diagram
bool showVoronoi = true;
// show convex hull
bool showConvexHull = true;
// show candidate points
bool showCandidatePoints = true;

// vertex shader source
const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    out vec3 ourColor;

    void main() {
        gl_Position = vec4(aPos, 1.0);
        ourColor = aColor;
    }
)glsl";

// fragment shader source
const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;

    in vec3 ourColor;

    void main() {
        FragColor = vec4(ourColor, 1.0);
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
void setupBuffers(unsigned int* VAO, unsigned int* VBO, const std::vector<float>& vertices) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
}

// initialize window and context
GLFWwindow* initWindowAndContext() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(640, 640, "Largest empty circle", NULL, NULL);
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

// function that returns a vector of points that represent the vertices of the Voronoi diagram
std::vector<Point_2> getCGALPoints(std::vector<float> vertices){
    std::vector<Point_2> points;
    for (size_t i = 0; i < vertices.size(); i += 6) {
        points.push_back(Point_2(vertices[i], vertices[i + 1]));
    }
    return points;
}

// function that receves a vector of points gladPoints and calculates the Voronoi diagram, the convex hull and the largest empty circle,
// then returns two vectors of floats that represent the vertices and edges of the whole figure
std::tuple<std::vector<float>, std::vector<float>, std::vector<float>> getLargestEmptyCircle(std::vector<float> inputPointsGLAD) {
    // this vector will hold the GLAD output points vertices
    std::vector<float> outputPointsVerticesGLAD;
    // this vector will hold the GLAD output edges vertices
    std::vector<float> outputEdgesVerticesGLAD;

    // 1- delanuay triangulation and voronoi diagram
    // the input points are stored in a vector of CGAL points
    std::vector<Point_2> inputPointsCGAL = getCGALPoints(inputPointsGLAD);
    // the triangulation object is created
    Delaunay_triangulation_2 dt2;
    // the points are inserted in the triangulation (this will also compute the Voronoi diagram)
    dt2.insert(inputPointsCGAL.begin(), inputPointsCGAL.end());
    // a box from (-1,-1) to (1,1) is created, this will crop the Voronoi diagram
    Iso_rectangle_2 bbox(-1,-1,1,1);
    // the cropped Voronoi diagram is created
    Cropped_voronoi_from_delaunay voronoi(bbox);
    dt2.draw_dual(voronoi);

    // vector for the GLAD point vertices of the Voronoi diagram
    std::vector<float> voronoiVerticesGLAD;
    // vector for the GLAD edge vertices of the Voronoi diagram
    std::vector<float> voronoiEdgesGLAD;
    // set with the CGAL Point_2 vertices of the Voronoi diagram
    std::set<Point_2> voronoiVerticesCGAL;

    // for all segments in the list
    for (const Segment_2& segment : voronoi.m_cropped_vd) {
        // the source and target points of the segment are extracted
        Point_2 source = segment.source();
        Point_2 target = segment.target();
        // the source and target points are added to the set
        voronoiVerticesCGAL.insert(source);
        voronoiVerticesCGAL.insert(target);

        // if the voronoi diagram is going to be shown
        if (showVoronoi) {
            // the coordinates are converted to float and added to the vector
            voronoiEdgesGLAD.push_back(CGAL::to_double(source.x()));
            voronoiEdgesGLAD.push_back(CGAL::to_double(source.y()));
            voronoiEdgesGLAD.push_back(0.0f);
            // the color of the edges is set
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[0]);
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[1]);
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[2]);
            // the coordinates are converted to float and added to the vector
            voronoiEdgesGLAD.push_back(CGAL::to_double(target.x()));
            voronoiEdgesGLAD.push_back(CGAL::to_double(target.y()));
            voronoiEdgesGLAD.push_back(0.0f);
            // the color of the edges is set
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[0]);
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[1]);
            voronoiEdgesGLAD.push_back(voronoiEdgesColor[2]);
        } 
    }

    // 2- convex hull
    std::vector<std::size_t> ch_points(inputPointsCGAL.size()), out;
    std::iota(ch_points.begin(), ch_points.end(), 0);
    // the convex hull is computed
    CGAL::convex_hull_2(ch_points.begin(), ch_points.end(), std::back_inserter(out), Convex_hull_traits_2(CGAL::make_property_map(inputPointsCGAL)));
    // vector for the vertices of the convex hull, only with the x and y coordinates
    std::vector<float> convexHullVerticesXY;
    // a polygon is created with the convex hull vertices
    Polygon_2 ch;
    // for all vertices in out
    for (std::size_t i : out) {
        // the coordinates are converted to float and added to the vector
        convexHullVerticesXY.push_back(CGAL::to_double(inputPointsCGAL[i].x()));
        convexHullVerticesXY.push_back(CGAL::to_double(inputPointsCGAL[i].y()));
        // the vertex is added to the polygon
        ch.push_back(inputPointsCGAL[i]);
    }
    // vector for the edges of the convex hull
    std::vector<float> convexHullEdgesGLAD;
    // vector of segments that represent the edges of the convex hull
    std::vector<Segment_2> chSegments;
    // for all vertices in convexHullVerticesXY
    for (size_t i = 0; i < convexHullVerticesXY.size(); i += 2) {
        // if the convex hull is going to be shown
        if (showConvexHull) {
            // the coordinates are added to the edges vector
            convexHullEdgesGLAD.push_back(convexHullVerticesXY[i]);
            convexHullEdgesGLAD.push_back(convexHullVerticesXY[i + 1]);
            convexHullEdgesGLAD.push_back(0.0f);
            convexHullEdgesGLAD.push_back(convexHullColor[0]);
            convexHullEdgesGLAD.push_back(convexHullColor[1]);
            convexHullEdgesGLAD.push_back(convexHullColor[2]);
            // the next vertex is added to the edges vector
            convexHullEdgesGLAD.push_back(convexHullVerticesXY[(i + 2) % convexHullVerticesXY.size()]);
            convexHullEdgesGLAD.push_back(convexHullVerticesXY[(i + 3) % convexHullVerticesXY.size()]);
            convexHullEdgesGLAD.push_back(0.0f);
            convexHullEdgesGLAD.push_back(convexHullColor[0]);
            convexHullEdgesGLAD.push_back(convexHullColor[1]);
            convexHullEdgesGLAD.push_back(convexHullColor[2]);
        }

        // the segment is added to the segments vector
        chSegments.push_back(Segment_2(Point_2(convexHullVerticesXY[i], convexHullVerticesXY[i + 1]), Point_2(convexHullVerticesXY[(i + 2) % convexHullVerticesXY.size()], convexHullVerticesXY[(i + 3) % convexHullVerticesXY.size()])));
    }

    // 3- candidate points
    // Point_2 vector for the candidate points
    std::vector<Point_2> candidatePoints;
    // the candidate vertices of the Voronoi diagram are added to the edges vector
    for (const Point_2& vertex : voronoiVerticesCGAL) {
        // if vertex is inside the convex hull, it's added to the edges vector
        // here the opposite is checked so points in the boundary are also added
        if (!ch.has_on_unbounded_side(vertex)) {
            // the vertex is added to the edges vector
            // if the candidate points are going to be shown
            if (showCandidatePoints) {
                // position
                voronoiVerticesGLAD.push_back(CGAL::to_double(vertex.x()));
                voronoiVerticesGLAD.push_back(CGAL::to_double(vertex.y()));
                voronoiVerticesGLAD.push_back(0.0f);
                // color
                voronoiVerticesGLAD.push_back(candidatePointsColor[0]);
                voronoiVerticesGLAD.push_back(candidatePointsColor[1]);
                voronoiVerticesGLAD.push_back(candidatePointsColor[2]);
            }
            /// the vertex is added to the candidate points vector
            candidatePoints.push_back(vertex);
        }
    }

    // for all segments in the convex hull
    for (const Segment_2& chSegment : chSegments) {
        // for all segments in the cropped Voronoi diagram
        for (const Segment_2& voronoiSegment : voronoi.m_cropped_vd) {
            // the intersection of the segments is stored in obj which supports multiple types
            CGAL::Object obj = CGAL::intersection(chSegment, voronoiSegment);
            // if obj is a point, it is stored in the list
            const Point_2* p = CGAL::object_cast<Point_2>(&obj);
            if (p) {
                if (showCandidatePoints) {
                    // the coordinates are converted to float and added to the vector
                    voronoiVerticesGLAD.push_back(CGAL::to_double(p->x()));
                    voronoiVerticesGLAD.push_back(CGAL::to_double(p->y()));
                    voronoiVerticesGLAD.push_back(0.0f);
                    // the color of the edges is set
                    voronoiVerticesGLAD.push_back(candidatePointsColor[0]);
                    voronoiVerticesGLAD.push_back(candidatePointsColor[1]);
                    voronoiVerticesGLAD.push_back(candidatePointsColor[2]);
                }
                // the vertex is added to the candidate points vector
                candidatePoints.push_back(*p);
            }
        }
    }

    // 4- largest empty circle
    // the largest empty circle is computed
    // the center and squared radius of the largest empty circle are stored in the variables
    Point_2 center;
    K::FT squared_radius = 0;

    // for all candidate points
    for (const Point_2& candidatePoint : candidatePoints) {
        // the nearest neighbor of the candidate point is stored in the nearest_neighbor variable
        Point_2 nearest_neighbor = dt2.nearest_vertex(candidatePoint)->point();
        // the squared distance between the candidate point and the nearest neighbor is stored in the squared_distance variable
        K::FT squared_distance = CGAL::squared_distance(candidatePoint, nearest_neighbor);
        // if the squared distance is greater than the squared radius
        if (squared_distance > squared_radius) {
            // the squared radius is set to the squared distance
            squared_radius = squared_distance;
            // the center is set to the candidate point
            center = candidatePoint;
        }
    }

    // the largest empty circle is drawn
    // the squared radius is converted to float
    float radius = std::sqrt(CGAL::to_double(squared_radius));
    // the center is converted to float and added to the vector
    voronoiVerticesGLAD.push_back(CGAL::to_double(center.x()));
    voronoiVerticesGLAD.push_back(CGAL::to_double(center.y()));
    voronoiVerticesGLAD.push_back(0.0f);
    // the color of the edges is set
    voronoiVerticesGLAD.push_back(largestEmptyCircleColor[0]);
    voronoiVerticesGLAD.push_back(largestEmptyCircleColor[1]);
    voronoiVerticesGLAD.push_back(largestEmptyCircleColor[2]);

    // the vertices of the largest empty circle are stored a new vector
    std::vector<float> largestEmptyCircleVerticesGLAD;
    // the number of segments that will be used to draw the largest empty circle
    int N = 40;
    // the angle between each segment
    float angleIncrement = (2.0f * M_PI) / N;

    // for all segments
    for (int i = 0; i < N; i++) {
        // the angle is calculated
        float angle = i * angleIncrement;
        // the x and y coordinates of the segment are calculated
        float x = CGAL::to_double(center.x()) + radius * std::cos(angle);
        float y = CGAL::to_double(center.y()) + radius * std::sin(angle);
        // the coordinates are added to the vector
        largestEmptyCircleVerticesGLAD.push_back(x);
        largestEmptyCircleVerticesGLAD.push_back(y);
        largestEmptyCircleVerticesGLAD.push_back(0.0f);
        // the color of the edges is set
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[0]);
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[1]);
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[2]);
        // the next angle is calculated
        angle = (i + 1) * angleIncrement;
        // the x and y coordinates of the segment are calculated
        x = CGAL::to_double(center.x()) + radius * std::cos(angle);
        y = CGAL::to_double(center.y()) + radius * std::sin(angle);
        // the coordinates are added to the vector
        largestEmptyCircleVerticesGLAD.push_back(x);
        largestEmptyCircleVerticesGLAD.push_back(y);
        largestEmptyCircleVerticesGLAD.push_back(0.0f);
        // the color of the edges is set
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[0]);
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[1]);
        largestEmptyCircleVerticesGLAD.push_back(largestEmptyCircleColor[2]);
    }

    // to GLAD
    // vertices
    // the original points are added to the output points vertices
    outputPointsVerticesGLAD.insert(outputPointsVerticesGLAD.end(), inputPointsGLAD.begin(), inputPointsGLAD.end());
    // the vertices of the Voronoi diagram are added to the output points vertices
    outputPointsVerticesGLAD.insert(outputPointsVerticesGLAD.end(), voronoiVerticesGLAD.begin(), voronoiVerticesGLAD.end());

    // edges
    // the edges of the Voronoi diagram are added to the output edges vertices
    outputEdgesVerticesGLAD.insert(outputEdgesVerticesGLAD.end(), voronoiEdgesGLAD.begin(), voronoiEdgesGLAD.end());
    // the edges of the convex hull are added to the output edges vertices
    outputEdgesVerticesGLAD.insert(outputEdgesVerticesGLAD.end(), convexHullEdgesGLAD.begin(), convexHullEdgesGLAD.end());

    return std::make_tuple(outputPointsVerticesGLAD, outputEdgesVerticesGLAD, largestEmptyCircleVerticesGLAD);
}

// function to transform a Point_2 in an specified range to a Point_2 in the range [-1, 1] and with a padding that is specified by the user
Point_2 transformPointToRange(Point_2 point, float minX, float minY, float maxX, float maxY) {
    // dimensions of the bounding box are calculated
    float width = maxX - minX;
    float height = maxY - minY;

    // the scale factor to fit the shape within [-1, 1] range is calculated, preserving aspect ratio
    float scaleX = 2.0f / width;
    float scaleY = 2.0f / height;
    float scale = std::min(scaleX, scaleY); // Use the smaller scale to ensure fit

    // the new coordinates are calculated
    float x = scale * (CGAL::to_double(point.x()) - minX) - (scale * width) / 2.0f;
    float y = scale * (CGAL::to_double(point.y()) - minY) - (scale * height) / 2.0f;

    // the coordinates are centered
    x = (x + 1.0f) / 2.0f * 2.0f - 1.0f; // horizontally
    y = (y + 1.0f) / 2.0f * 2.0f - 1.0f; // vertically

    return Point_2(x, y);
}

// funtion that reads the input points from a geojson file and returns a vector of floats that represent the vertices of the points
std::vector<float> readInputPointsFrom() {
    // vector for the points vertices
    std::vector<float> pointVertices;
    // vector for the cgal points from the geojson file
    std::vector<Point_2> pointsCGALRaw;

    // ask the user if they want to show the Voronoi diagram
    std::cout << "Do you want to show the Voronoi diagram? (y/n): ";
    char showVoronoiChar;
    std::cin >> showVoronoiChar;
    // if the user wants to show the Voronoi diagram, the boolean is set to true
    if (showVoronoiChar == 'y') showVoronoi = true;
    // if the user doesn't want to show the Voronoi diagram, the boolean is set to false
    else showVoronoi = false;

    // ask the user if they want to show the convex hull
    std::cout << "Do you want to show the convex hull? (y/n): ";
    char showConvexHullChar;
    std::cin >> showConvexHullChar;
    // if the user wants to show the convex hull, the boolean is set to true
    if (showConvexHullChar == 'y') showConvexHull = true;
    // if the user doesn't want to show the convex hull, the boolean is set to false
    else showConvexHull = false;

    // ask the user if they want to show the candidate points
    std::cout << "Do you want to show the candidate points? (y/n): ";
    char showCandidatePointsChar;
    std::cin >> showCandidatePointsChar;
    // if the user wants to show the candidate points, the boolean is set to true
    if (showCandidatePointsChar == 'y') showCandidatePoints = true;
    // if the user doesn't want to show the candidate points, the boolean is set to false
    else showCandidatePoints = false;

    // the user can pick the file to read the input points from
    // for the boundary
    std::string filename;
    std::cout << "Enter the geojson file route for the boundary: ";
    std::cin >> filename;

    // using nlohmann json library to read the input points from a json file
    using json = nlohmann::json;

    // the file is opened
    std::ifstream file(filename);
    // the file is parsed
    json j;
    file >> j;
    // the file is closed
    file.close();

    // for all coordinates in the file
    for (size_t i = 0; i < j["features"][0]["geometry"]["coordinates"][0].size(); i++) {
        // the x and y coordinates are extracted
        float x = j["features"][0]["geometry"]["coordinates"][0][i][0];
        float y = j["features"][0]["geometry"]["coordinates"][0][i][1];
        // the position is added to the cgal points vector
        pointsCGALRaw.push_back(Point_2(x, y));
    }

    // for the points inside the boundary
    std::cout << "Enter the geojson file route for the points inside the boundary: ";
    std::cin >> filename;

    // the file is opened
    file.open(filename);
    // the file is parsed
    file >> j;
    // the file is closed
    file.close();

    // for all features in the file
    for (size_t i = 0; i < j["features"].size(); i++) {
        // if ["geometry"]["type"] is "Polygon", only the first coordinates is considered
        if (j["features"][i]["geometry"]["type"] == "Polygon") {
            // for all coordinates in the file
            float x = j["features"][i]["geometry"]["coordinates"][0][0][0];
            float y = j["features"][i]["geometry"]["coordinates"][0][0][1];
        }
        // if ["geometry"]["type"] is "Point", the coordinates are extracted
        else if (j["features"][i]["geometry"]["type"] == "Point") {
            float x = j["features"][i]["geometry"]["coordinates"][0];
            float y = j["features"][i]["geometry"]["coordinates"][1];
            // the position is added to the cgal points vector
            pointsCGALRaw.push_back(Point_2(x, y));
        }
    }

    // the minimum and maximum x and y coordinates are calculated
    auto minX = pointsCGALRaw[0].x();
    auto minY = pointsCGALRaw[0].y();
    auto maxX = pointsCGALRaw[0].x();
    auto maxY = pointsCGALRaw[0].y();

    // for all points in the cgal points vector
    for (const Point_2& point : pointsCGALRaw) {
        // the minimum x and y coordinates are updated
        if (point.x() < minX) minX = point.x();
        if (point.y() < minY) minY = point.y();
        // the maximum x and y coordinates are updated
        if (point.x() > maxX) maxX = point.x();
        if (point.y() > maxY) maxY = point.y();
    }

    // a vector for the cgal points in the range [-1, 1] is created
    std::vector<Point_2> pointsCGAL;
    // for all points in the cgal points vector
    for (const Point_2& point : pointsCGALRaw) {
        // the point is transformed to the range [-1, 1] and added to the points vector
        pointsCGAL.push_back(transformPointToRange(point, minX, minY, maxX, maxY));
    }

    // the points are added to pointVertices
    for (const Point_2& point : pointsCGAL) {
        // position
        pointVertices.push_back(CGAL::to_double(point.x()));
        pointVertices.push_back(CGAL::to_double(point.y()));
        pointVertices.push_back(0.0f);
        // color
        pointVertices.push_back(inputPointsColor[0]);
        pointVertices.push_back(inputPointsColor[1]);
        pointVertices.push_back(inputPointsColor[2]);
    }

    return pointVertices;
}

int main(int, char**) {
    // Seed the random number generator with the current time
    srand(static_cast<unsigned int>(time(NULL)));

    // Initialize window and context
    GLFWwindow* window = initWindowAndContext();
    // if window or context initialization failed, return -1
    if (!window) return -1;

    // Create shader program
    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    // read from readInputPointsFrom() to read the input points from a geojson file
    std::vector<float> pointVertices = readInputPointsFrom();

    // // vector for the points vertices
    // std::vector<float> pointVertices;
    // for (int i = 0; i < n; i++) {
    //     float x = -1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f)));
    //     float y = -1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (2.0f)));
    //     // position
    //     pointVertices.push_back(x);
    //     pointVertices.push_back(y);
    //     pointVertices.push_back(0.0f);
    //     // color
    //     pointVertices.push_back(inputPointsColor[0]);
    //     pointVertices.push_back(inputPointsColor[1]);
    //     pointVertices.push_back(inputPointsColor[2]);
    // }

    // vector for the line vertices
    std::vector<float> lineVertices;
    // vector for the largest empty circle vertices
    std::vector<float> circleVertices;

    // the processed data is obtained
    std::tuple<std::vector<float>, std::vector<float>, std::vector<float>> largestEmptyCircle = getLargestEmptyCircle(pointVertices);

    // the points vertices are set
    pointVertices = std::get<0>(largestEmptyCircle);
    // the edges vertices are set
    lineVertices = std::get<1>(largestEmptyCircle);
    // the largest empty circle vertices are set
    circleVertices = std::get<2>(largestEmptyCircle);

    // buffer for the vertices
    unsigned int pointVAO, pointVBO;
    setupBuffers(&pointVAO, &pointVBO, pointVertices);

    // buffer for the edges
    unsigned int lineVAO, lineVBO;
    setupBuffers(&lineVAO, &lineVBO, lineVertices);

    // buffer for the largest empty circle
    unsigned int circleVAO, circleVBO;
    setupBuffers(&circleVAO, &circleVBO, circleVertices);

    // the size of the points is set to 10.0f
    glPointSize(5.0f);

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
        glDrawArrays(GL_POINTS, 0, pointVertices.size() / 6);

        // the edges are drawn
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, lineVertices.size() / 6);

        // the largest empty circle is drawn
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_LINES, 0, circleVertices.size() / 6);

        glfwSwapBuffers(window);
    }

    // clean up
    glDeleteVertexArrays(1, &pointVAO);
    glDeleteBuffers(1, &pointVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}