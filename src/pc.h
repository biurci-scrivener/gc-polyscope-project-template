#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <iostream>
#include <numeric>
#include <algorithm>

#include "polyscope/polyscope.h"
#include "polyscope/point_cloud.h"
#include "polyscope/curve_network.h"

#include "../deps/libigl/include/igl/bounding_box.h"

#include <queue>
#include <unordered_map>
#include <stdexcept>

std::tuple<Eigen::VectorXi, Eigen::VectorXi, std::vector<std::vector<int>>, Eigen::MatrixXd> appendBoundaryAndCage(Eigen::MatrixXd &P, Eigen::MatrixXd &N);

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
