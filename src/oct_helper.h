#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <iostream>
#include <numeric>
#include <algorithm>

#include "polyscope/polyscope.h"
#include "polyscope/point_cloud.h"
#include "polyscope/curve_network.h"

#include "../deps/libigl/include/igl/octree.h"
#include "../deps/libigl/include/igl/knn.h"
#include "../deps/libigl/include/igl/bounding_box.h"

#include <queue>
#include <unordered_map>
#include <stdexcept>

struct FaradayOctree {

    // unused: need to implement this to unify things in the future

    Eigen::MatrixXd P;
	Eigen::MatrixXd N;

    Eigen::VectorXi is_boundary_point;
	Eigen::VectorXi is_cage_point;
	std::vector<std::vector<int>> my_cage_points;
	Eigen::MatrixXd bb;

    // defined on original octree cells

    std::vector<std::vector<int >> PI; // associates points to octree cells
	Eigen::MatrixXi CH; // cells * 8, lists children
	Eigen::MatrixXd CN; // cell centers
	Eigen::VectorXd W; //  cell widths

    // defined only on leaf cells

    std::vector<std::vector<int >> PI_l;
	// no Eigen::MatrixXi CH_l: doesn't make sense
	Eigen::MatrixXd CN_l;
	Eigen::VectorXd W_l;
	std::unordered_map<int, int> leaf_to_all;
	std::unordered_map<int, int> all_to_leaf;
	Eigen::VectorXi depths;
	Eigen::VectorXi depths_l;
	Eigen::VectorXi parents;
	Eigen::VectorXi parents_l;
    std::vector<struct CellNeighbors> neighs;
	Eigen::VectorXi is_boundary_cell;
	Eigen::VectorXi is_cage_cell;

};

struct CellNeighbors {
    std::vector<int> right;
    std::vector<int> left;
    std::vector<int> top;
    std::vector<int> bottom;
    std::vector<int> front;
    std::vector<int> back;
    std::vector<int> all;
};

double getFunctionValueAtNeighbor(  int leaf, std::vector<int> &neighs, Eigen::VectorXd &W_all, Eigen::MatrixXi &CH, 
                                                Eigen::VectorXi &parents, std::unordered_map<int, int> &all_to_leaf, std::unordered_map<int, int> &leaf_to_all,
                                                Eigen::VectorXd &f
                                                );

std::tuple<Eigen::VectorXd, double> getNeighRep(int leaf, std::vector<int> &neighs, size_t n, Eigen::MatrixXd &CN, Eigen::VectorXd &W, int side);
std::tuple<Eigen::VectorXd, double, double> getNeighRep(int leaf, std::vector<int> &neighs, size_t n, Eigen::MatrixXd &CN, Eigen::VectorXd &W, int side, Eigen::VectorXd &f);

std::tuple<std::vector<struct CellNeighbors>, Eigen::VectorXi, Eigen::VectorXi> createLeafNeighbors(std::vector<std::vector<int>> &PI, Eigen::MatrixXd &CN, Eigen::VectorXd &W, Eigen::VectorXi &is_cage_point, std::vector<Eigen::Vector3d> &oc_pts, std::vector<Eigen::Vector2i> &oc_edges, Eigen::MatrixXd &bb_oct);

std::tuple< std::vector<std::vector<int>>,
            Eigen::MatrixXd, 
            Eigen::VectorXd,
            std::unordered_map<int, int>, 
            std::unordered_map<int, int>,
            Eigen::VectorXi,
            Eigen::VectorXi,
            Eigen::VectorXi,
            Eigen::VectorXi> 
getLeaves(Eigen::MatrixXd &P,
            std::vector<std::vector<int>> &PI, 
            Eigen::MatrixXi &CH, 
            Eigen::MatrixXd &CN, 
            Eigen::VectorXd &W);

Eigen::MatrixXd octreeBB(Eigen::MatrixXd &CN);

std::tuple<Eigen::VectorXi, Eigen::VectorXi, std::vector<std::vector<int>>, Eigen::MatrixXd> appendBoundaryAndCage(Eigen::MatrixXd &P, Eigen::MatrixXd &N);

std::tuple<std::vector<Eigen::Vector3d>, std::vector<Eigen::Vector2i>> visOctree(Eigen::MatrixXd CN, Eigen::VectorXd W);

inline const Eigen::MatrixXd ico_pts = (Eigen::MatrixXd(12, 3) << 
            0,      0.52573,      0.85065,
            0,     -0.52573,      0.85065,
            0,      0.52573,     -0.85065,
            0,     -0.52573,     -0.85065,
      0.52573,      0.85065,            0,
     -0.52573,      0.85065,            0,
      0.52573,     -0.85065,            0,
     -0.52573,     -0.85065,            0,
      0.85065,            0,      0.52573,
     -0.85065,            0,      0.52573,
      0.85065,            0,     -0.52573,
     -0.85065,            0,     -0.52573).finished();

inline const Eigen::MatrixXd ico_pts_1 = (Eigen::MatrixXd(42, 3) << 
            0,      0.52573,      0.85065,
            0,     -0.52573,      0.85065,
            0,      0.52573,     -0.85065,
            0,     -0.52573,     -0.85065,
      0.52573,      0.85065,            0,
     -0.52573,      0.85065,            0,
      0.52573,     -0.85065,            0,
     -0.52573,     -0.85065,            0,
      0.85065,            0,      0.52573,
     -0.85065,            0,      0.52573,
      0.85065,            0,     -0.52573,
     -0.85065,            0,     -0.52573,
            0,            0,            1,
      0.30902,      0.80902,          0.5,
     -0.30902,      0.80902,          0.5,
          0.5,      0.30902,      0.80902,
         -0.5,      0.30902,      0.80902,
      0.30902,     -0.80902,          0.5,
     -0.30902,     -0.80902,          0.5,
          0.5,     -0.30902,      0.80902,
         -0.5,     -0.30902,      0.80902,
            0,            0,           -1,
      0.30902,      0.80902,         -0.5,
     -0.30902,      0.80902,         -0.5,
          0.5,      0.30902,     -0.80902,
         -0.5,      0.30902,     -0.80902,
      0.30902,     -0.80902,         -0.5,
     -0.30902,     -0.80902,         -0.5,
          0.5,     -0.30902,     -0.80902,
         -0.5,     -0.30902,     -0.80902,
            0,            1,            0,
      0.80902,          0.5,      0.30902,
      0.80902,          0.5,     -0.30902,
     -0.80902,          0.5,      0.30902,
     -0.80902,          0.5,     -0.30902,
            0,           -1,            0,
      0.80902,         -0.5,      0.30902,
      0.80902,         -0.5,     -0.30902,
     -0.80902,         -0.5,      0.30902,
     -0.80902,         -0.5,     -0.30902,
            1,            0,            0,
           -1,            0,            0).finished();

inline const Eigen::MatrixXd ico_pts_2 = (Eigen::MatrixXd(162, 3) << 
            0,      0.52573,      0.85065,
            0,     -0.52573,      0.85065,
            0,      0.52573,     -0.85065,
            0,     -0.52573,     -0.85065,
      0.52573,      0.85065,            0,
     -0.52573,      0.85065,            0,
      0.52573,     -0.85065,            0,
     -0.52573,     -0.85065,            0,
      0.85065,            0,      0.52573,
     -0.85065,            0,      0.52573,
      0.85065,            0,     -0.52573,
     -0.85065,            0,     -0.52573,
            0,            0,            1,
      0.30902,      0.80902,          0.5,
     -0.30902,      0.80902,          0.5,
          0.5,      0.30902,      0.80902,
         -0.5,      0.30902,      0.80902,
      0.30902,     -0.80902,          0.5,
     -0.30902,     -0.80902,          0.5,
          0.5,     -0.30902,      0.80902,
         -0.5,     -0.30902,      0.80902,
            0,            0,           -1,
      0.30902,      0.80902,         -0.5,
     -0.30902,      0.80902,         -0.5,
          0.5,      0.30902,     -0.80902,
         -0.5,      0.30902,     -0.80902,
      0.30902,     -0.80902,         -0.5,
     -0.30902,     -0.80902,         -0.5,
          0.5,     -0.30902,     -0.80902,
         -0.5,     -0.30902,     -0.80902,
            0,            1,            0,
      0.80902,          0.5,      0.30902,
      0.80902,          0.5,     -0.30902,
     -0.80902,          0.5,      0.30902,
     -0.80902,          0.5,     -0.30902,
            0,           -1,            0,
      0.80902,         -0.5,      0.30902,
      0.80902,         -0.5,     -0.30902,
     -0.80902,         -0.5,      0.30902,
     -0.80902,         -0.5,     -0.30902,
            1,            0,            0,
           -1,            0,            0,
            0,      0.27327,      0.96194,
      0.16062,      0.69378,      0.70205,
     -0.16062,      0.69378,      0.70205,
      0.25989,      0.43389,      0.86267,
     -0.25989,      0.43389,      0.86267,
            0,     -0.27327,      0.96194,
      0.16062,     -0.69378,      0.70205,
     -0.16062,     -0.69378,      0.70205,
      0.25989,     -0.43389,      0.86267,
     -0.25989,     -0.43389,      0.86267,
            0,      0.27327,     -0.96194,
      0.16062,      0.69378,     -0.70205,
     -0.16062,      0.69378,     -0.70205,
      0.25989,      0.43389,     -0.86267,
     -0.25989,      0.43389,     -0.86267,
            0,     -0.27327,     -0.96194,
      0.16062,     -0.69378,     -0.70205,
     -0.16062,     -0.69378,     -0.70205,
      0.25989,     -0.43389,     -0.86267,
     -0.25989,     -0.43389,     -0.86267,
      0.43389,      0.86267,      0.25989,
      0.43389,      0.86267,     -0.25989,
      0.27327,      0.96194,            0,
      0.69378,      0.70205,      0.16062,
      0.69378,      0.70205,     -0.16062,
     -0.43389,      0.86267,      0.25989,
     -0.43389,      0.86267,     -0.25989,
     -0.27327,      0.96194,            0,
     -0.69378,      0.70205,      0.16062,
     -0.69378,      0.70205,     -0.16062,
      0.43389,     -0.86267,      0.25989,
      0.43389,     -0.86267,     -0.25989,
      0.27327,     -0.96194,            0,
      0.69378,     -0.70205,      0.16062,
      0.69378,     -0.70205,     -0.16062,
     -0.43389,     -0.86267,      0.25989,
     -0.43389,     -0.86267,     -0.25989,
     -0.27327,     -0.96194,            0,
     -0.69378,     -0.70205,      0.16062,
     -0.69378,     -0.70205,     -0.16062,
      0.70205,      0.16062,      0.69378,
      0.70205,     -0.16062,      0.69378,
      0.86267,      0.25989,      0.43389,
      0.86267,     -0.25989,      0.43389,
      0.96194,            0,      0.27327,
     -0.70205,      0.16062,      0.69378,
     -0.70205,     -0.16062,      0.69378,
     -0.86267,      0.25989,      0.43389,
     -0.86267,     -0.25989,      0.43389,
     -0.96194,            0,      0.27327,
      0.70205,      0.16062,     -0.69378,
      0.70205,     -0.16062,     -0.69378,
      0.86267,      0.25989,     -0.43389,
      0.86267,     -0.25989,     -0.43389,
      0.96194,            0,     -0.27327,
     -0.70205,      0.16062,     -0.69378,
     -0.70205,     -0.16062,     -0.69378,
     -0.86267,      0.25989,     -0.43389,
     -0.86267,     -0.25989,     -0.43389,
     -0.96194,            0,     -0.27327,
      0.26287,      0.16246,      0.95106,
     -0.26287,      0.16246,      0.95106,
      0.26287,     -0.16246,      0.95106,
     -0.26287,     -0.16246,      0.95106,
            0,      0.85065,      0.52573,
      0.42533,      0.58779,      0.68819,
      0.16246,      0.95106,      0.26287,
      0.58779,      0.68819,      0.42533,
     -0.42533,      0.58779,      0.68819,
     -0.16246,      0.95106,      0.26287,
     -0.58779,      0.68819,      0.42533,
      0.52573,            0,      0.85065,
      0.68819,      0.42533,      0.58779,
     -0.52573,            0,      0.85065,
     -0.68819,      0.42533,      0.58779,
            0,     -0.85065,      0.52573,
      0.42533,     -0.58779,      0.68819,
      0.16246,     -0.95106,      0.26287,
      0.58779,     -0.68819,      0.42533,
     -0.42533,     -0.58779,      0.68819,
     -0.16246,     -0.95106,      0.26287,
     -0.58779,     -0.68819,      0.42533,
      0.68819,     -0.42533,      0.58779,
     -0.68819,     -0.42533,      0.58779,
      0.26287,      0.16246,     -0.95106,
     -0.26287,      0.16246,     -0.95106,
      0.26287,     -0.16246,     -0.95106,
     -0.26287,     -0.16246,     -0.95106,
            0,      0.85065,     -0.52573,
      0.42533,      0.58779,     -0.68819,
      0.16246,      0.95106,     -0.26287,
      0.58779,      0.68819,     -0.42533,
     -0.42533,      0.58779,     -0.68819,
     -0.16246,      0.95106,     -0.26287,
     -0.58779,      0.68819,     -0.42533,
      0.52573,            0,     -0.85065,
      0.68819,      0.42533,     -0.58779,
     -0.52573,            0,     -0.85065,
     -0.68819,      0.42533,     -0.58779,
            0,     -0.85065,     -0.52573,
      0.42533,     -0.58779,     -0.68819,
      0.16246,     -0.95106,     -0.26287,
      0.58779,     -0.68819,     -0.42533,
     -0.42533,     -0.58779,     -0.68819,
     -0.16246,     -0.95106,     -0.26287,
     -0.58779,     -0.68819,     -0.42533,
      0.68819,     -0.42533,     -0.58779,
     -0.68819,     -0.42533,     -0.58779,
      0.85065,      0.52573,            0,
      0.95106,      0.26287,      0.16246,
      0.95106,      0.26287,     -0.16246,
     -0.85065,      0.52573,            0,
     -0.95106,      0.26287,      0.16246,
     -0.95106,      0.26287,     -0.16246,
      0.85065,     -0.52573,            0,
      0.95106,     -0.26287,      0.16246,
      0.95106,     -0.26287,     -0.16246,
     -0.85065,     -0.52573,            0,
     -0.95106,     -0.26287,      0.16246,
     -0.95106,     -0.26287,     -0.16246).finished();

