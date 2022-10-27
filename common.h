//
// Created by CD on 2022/10/25.
//

#ifndef RT_COMMON_H
#define RT_COMMON_H

#include <QVector3D>
#include <limits>

class unit_vec3 : public QVector3D {
public:
    unit_vec3() : QVector3D() {}
    explicit unit_vec3(const QVector3D& vec) : QVector3D(vec) {
        this->normalize();
    }
};

using color_t = QVector3D;
using vec3_t = QVector3D;
using point_t = QVector3D;

constexpr auto g_infinity = std::numeric_limits<float>::max();

#endif //RT_COMMON_H
