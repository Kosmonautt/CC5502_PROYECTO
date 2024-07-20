#include <utility> 
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
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

// function that receves a vector of points Point_2 and calculates the Voronoi diagram, the convex hull and the largest empty circle,
// then prints the center and the radius of the largest empty circle 
void getLargestEmptyCircle(std::vector<Point_2> inputPointsCGAL) {
    // 1- delanuay triangulation and voronoi diagram
    // the triangulation object is created
    Delaunay_triangulation_2 dt2;
    // the points are inserted in the triangulation (this will also compute the Voronoi diagram)
    dt2.insert(inputPointsCGAL.begin(), inputPointsCGAL.end());
    // the minimum and maximum x and y coordinates are calculated
    auto minX = inputPointsCGAL[0].x();
    auto minY = inputPointsCGAL[0].y();
    auto maxX = inputPointsCGAL[0].x();
    auto maxY = inputPointsCGAL[0].y();
    // for all points in the cgal points vector
    for (const Point_2& point : inputPointsCGAL) {
        // the minimum x and y coordinates are updated
        if (point.x() < minX) minX = point.x();
        if (point.y() < minY) minY = point.y();
        // the maximum x and y coordinates are updated
        if (point.x() > maxX) maxX = point.x();
        if (point.y() > maxY) maxY = point.y();
    }
    // the bounding box bigger than the minimum and maximum x and y coordinates is created
    Iso_rectangle_2 bbox(minX - 1, minY - 1, maxX + 1, maxY + 1);
    // the cropped Voronoi diagram is created
    Cropped_voronoi_from_delaunay voronoi(bbox);
    dt2.draw_dual(voronoi);

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
    // vector of segments that represent the edges of the convex hull
    std::vector<Segment_2> chSegments;
    // for all vertices in convexHullVerticesXY
    for (size_t i = 0; i < convexHullVerticesXY.size(); i += 2) {
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

    // the center of the largest empty circle is printed
    std::cout << "Center of the largest empty circle: " << "Longitude: " << CGAL::to_double(center.x()) << " Latitude: " << CGAL::to_double(center.y()) << std::endl;
    // the radius of the largest empty circle is printed
    std::cout << "Radius of the largest empty circle: " << radius << std::endl;
}

// funtion that reads the input points from a geojson file and returns a vector of Point_2 with the points
std::vector<Point_2> readInputPointsFrom() {
    // vector for the cgal points from the geojson file
    std::vector<Point_2> pointsCGAL;

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
        pointsCGAL.push_back(Point_2(x, y));
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
            pointsCGAL.push_back(Point_2(x, y));
        }
    }

    return pointsCGAL;
}

int main(int, char**) {
    // read from readInputPointsFrom() to read the input points from a geojson file
    std::vector<Point_2> pointVertices = readInputPointsFrom();
    // the processed data is obtained
    getLargestEmptyCircle(pointVertices);
    return 0;
}