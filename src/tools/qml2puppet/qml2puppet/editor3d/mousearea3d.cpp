// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef QUICK3D_MODULE

#include "mousearea3d.h"

#include "generalhelper.h"

#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick3D/private/qquick3dcamera_p.h>
#include <QtQuick3D/private/qquick3dorthographiccamera_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercamera_p.h>
#include <QtCore/qmath.h>

namespace QmlDesigner {
namespace Internal {

static GeneralHelper *s_generalHelper = nullptr;

// Double precision vector for cases where float calculations can suffer from rounding errors
class DoubleVec3D {
public:
    DoubleVec3D() = default;
    DoubleVec3D(const QVector3D &v)
        : x(double(v.x())),
          y(double(v.y())),
          z(double(v.z()))
    {}
    DoubleVec3D(double xx, double yy, double zz)
        : x(xx),
          y(yy),
          z(zz)
    {}

    static double dotProduct(const DoubleVec3D &v1, const DoubleVec3D &v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    QVector3D toVec3() const { return {float(x), float(y), float(z)}; }

    DoubleVec3D normalized() const
    {
        double len = x * x + y * y + z * z;
        if (qFuzzyIsNull(len - 1.)) {
            return *this;
        } else if (!qFuzzyIsNull(len)) {
            double sqrtLen = std::sqrt(len);
            return DoubleVec3D(x / sqrtLen, y / sqrtLen, z / sqrtLen);
        } else {
            return {};
        }
    }

    double length() const
    {
        double len = x * x + y * y + z * z;
        return std::sqrt(len);
    }

    DoubleVec3D operator-() { return {-x, -y, -z}; }

    double x = 0.;
    double y = 0.;
    double z = 0.;
};



DoubleVec3D operator*(double factor, const DoubleVec3D &v)
{
    return DoubleVec3D(v.x * factor, v.y * factor, v.z * factor);
}

DoubleVec3D operator*(DoubleVec3D &v1, const DoubleVec3D &v2)
{
    return DoubleVec3D(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

DoubleVec3D operator+(const DoubleVec3D &v1, const DoubleVec3D &v2)
{
    return DoubleVec3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

DoubleVec3D operator-(const DoubleVec3D &v1, const DoubleVec3D &v2)
{
    return DoubleVec3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

DoubleVec3D operator/(const DoubleVec3D &v, double div)
{
    return DoubleVec3D(v.x / div, v.y / div, v.z / div);
}

DoubleVec3D operator/(const DoubleVec3D &v1, const DoubleVec3D &v2)
{
    return DoubleVec3D(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

// Limited functionality double precision matrix4x4 for cases where float calculations
// can suffer from rounding errors
class DoubleMat44 {
public:
    DoubleMat44()
    {
        // Default is identity matrix
        m[0][0] = 1.;
        m[0][1] = 0.;
        m[0][2] = 0.;
        m[0][3] = 0.;
        m[1][0] = 0.;
        m[1][1] = 1.;
        m[1][2] = 0.;
        m[1][3] = 0.;
        m[2][0] = 0.;
        m[2][1] = 0.;
        m[2][2] = 1.;
        m[2][3] = 0.;
        m[3][0] = 0.;
        m[3][1] = 0.;
        m[3][2] = 0.;
        m[3][3] = 1.;
    }

    DoubleMat44(const QMatrix4x4 &mat)
    {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j)
                m[i][j] = double(mat(j, i));
        }
    }

    QMatrix4x4 toQMatrix4x4()
    {
        return QMatrix4x4(float(m[0][0]), float(m[1][0]), float(m[2][0]), float(m[3][0]),
                float(m[0][1]), float(m[1][1]), float(m[2][1]), float(m[3][1]),
                float(m[0][2]), float(m[1][2]), float(m[2][2]), float(m[3][2]),
                float(m[0][3]), float(m[1][3]), float(m[2][3]), float(m[3][3]));
    }

    static inline double matrixDet2(const double m[4][4], int col0, int col1, int row0,  int row1)
    {
        return m[col0][row0] * m[col1][row1] - m[col0][row1] * m[col1][row0];
    }

    static inline double matrixDet3(const double m[4][4], int col0, int col1, int col2,
                                    int row0, int row1, int row2)
    {
        return m[col0][row0] * matrixDet2(m, col1, col2, row1, row2)
                - m[col1][row0] * matrixDet2(m, col0, col2, row1, row2)
                + m[col2][row0] * matrixDet2(m, col0, col1, row1, row2);
    }

    DoubleMat44 inverted()
    {
        DoubleMat44 inv;

        double det = matrixDet3(m, 0, 1, 2, 0, 1, 2);
        if (det != 0.) {
            det = 1. / det;

            inv.m[0][0] =  matrixDet2(m, 1, 2, 1, 2) * det;
            inv.m[0][1] = -matrixDet2(m, 0, 2, 1, 2) * det;
            inv.m[0][2] =  matrixDet2(m, 0, 1, 1, 2) * det;
            inv.m[0][3] = 0;
            inv.m[1][0] = -matrixDet2(m, 1, 2, 0, 2) * det;
            inv.m[1][1] =  matrixDet2(m, 0, 2, 0, 2) * det;
            inv.m[1][2] = -matrixDet2(m, 0, 1, 0, 2) * det;
            inv.m[1][3] = 0;
            inv.m[2][0] =  matrixDet2(m, 1, 2, 0, 1) * det;
            inv.m[2][1] = -matrixDet2(m, 0, 2, 0, 1) * det;
            inv.m[2][2] =  matrixDet2(m, 0, 1, 0, 1) * det;
            inv.m[2][3] = 0;
            inv.m[3][0] = -inv.m[0][0] * m[3][0] - inv.m[1][0] * m[3][1] - inv.m[2][0] * m[3][2];
            inv.m[3][1] = -inv.m[0][1] * m[3][0] - inv.m[1][1] * m[3][1] - inv.m[2][1] * m[3][2];
            inv.m[3][2] = -inv.m[0][2] * m[3][0] - inv.m[1][2] * m[3][1] - inv.m[2][2] * m[3][2];
            inv.m[3][3] = 1;
        }
        return inv;
    }

    DoubleVec3D transform(const DoubleVec3D &v)
    {
        DoubleVec3D ret;

        auto multCol = [&](int c, double d) {
            ret.x += m[c][0] * d;
            ret.y += m[c][1] * d;
            ret.z += m[c][2] * d;
        };

        multCol(0, v.x);
        multCol(1, v.y);
        multCol(2, v.z);
        multCol(3, 1.);

        return ret;
    }

    static DoubleMat44 matrixMultiD(const QMatrix4x4& m1, const QMatrix4x4& m2)
    {
        DoubleMat44 m1d(m1);
        DoubleMat44 m2d(m2);

        return matrixMultiD(m1d, m2d);
    }

    static DoubleMat44 matrixMultiD(const DoubleMat44& m1, const DoubleMat44& m2)
    {
        DoubleMat44 m;

        m.m[0][0] = m1.m[0][0] * m2.m[0][0]
                  + m1.m[1][0] * m2.m[0][1]
                  + m1.m[2][0] * m2.m[0][2]
                  + m1.m[3][0] * m2.m[0][3];
        m.m[0][1] = m1.m[0][1] * m2.m[0][0]
                  + m1.m[1][1] * m2.m[0][1]
                  + m1.m[2][1] * m2.m[0][2]
                  + m1.m[3][1] * m2.m[0][3];
        m.m[0][2] = m1.m[0][2] * m2.m[0][0]
                  + m1.m[1][2] * m2.m[0][1]
                  + m1.m[2][2] * m2.m[0][2]
                  + m1.m[3][2] * m2.m[0][3];
        m.m[0][3] = m1.m[0][3] * m2.m[0][0]
                  + m1.m[1][3] * m2.m[0][1]
                  + m1.m[2][3] * m2.m[0][2]
                  + m1.m[3][3] * m2.m[0][3];

        m.m[1][0] = m1.m[0][0] * m2.m[1][0]
                  + m1.m[1][0] * m2.m[1][1]
                  + m1.m[2][0] * m2.m[1][2]
                  + m1.m[3][0] * m2.m[1][3];
        m.m[1][1] = m1.m[0][1] * m2.m[1][0]
                  + m1.m[1][1] * m2.m[1][1]
                  + m1.m[2][1] * m2.m[1][2]
                  + m1.m[3][1] * m2.m[1][3];
        m.m[1][2] = m1.m[0][2] * m2.m[1][0]
                  + m1.m[1][2] * m2.m[1][1]
                  + m1.m[2][2] * m2.m[1][2]
                  + m1.m[3][2] * m2.m[1][3];
        m.m[1][3] = m1.m[0][3] * m2.m[1][0]
                  + m1.m[1][3] * m2.m[1][1]
                  + m1.m[2][3] * m2.m[1][2]
                  + m1.m[3][3] * m2.m[1][3];

        m.m[2][0] = m1.m[0][0] * m2.m[2][0]
                  + m1.m[1][0] * m2.m[2][1]
                  + m1.m[2][0] * m2.m[2][2]
                  + m1.m[3][0] * m2.m[2][3];
        m.m[2][1] = m1.m[0][1] * m2.m[2][0]
                  + m1.m[1][1] * m2.m[2][1]
                  + m1.m[2][1] * m2.m[2][2]
                  + m1.m[3][1] * m2.m[2][3];
        m.m[2][2] = m1.m[0][2] * m2.m[2][0]
                  + m1.m[1][2] * m2.m[2][1]
                  + m1.m[2][2] * m2.m[2][2]
                  + m1.m[3][2] * m2.m[2][3];
        m.m[2][3] = m1.m[0][3] * m2.m[2][0]
                  + m1.m[1][3] * m2.m[2][1]
                  + m1.m[2][3] * m2.m[2][2]
                  + m1.m[3][3] * m2.m[2][3];

        m.m[3][0] = m1.m[0][0] * m2.m[3][0]
                  + m1.m[1][0] * m2.m[3][1]
                  + m1.m[2][0] * m2.m[3][2]
                  + m1.m[3][0] * m2.m[3][3];
        m.m[3][1] = m1.m[0][1] * m2.m[3][0]
                  + m1.m[1][1] * m2.m[3][1]
                  + m1.m[2][1] * m2.m[3][2]
                  + m1.m[3][1] * m2.m[3][3];
        m.m[3][2] = m1.m[0][2] * m2.m[3][0]
                  + m1.m[1][2] * m2.m[3][1]
                  + m1.m[2][2] * m2.m[3][2]
                  + m1.m[3][2] * m2.m[3][3];
        m.m[3][3] = m1.m[0][3] * m2.m[3][0]
                  + m1.m[1][3] * m2.m[3][1]
                  + m1.m[2][3] * m2.m[3][2]
                  + m1.m[3][3] * m2.m[3][3];

        return m;
    }

    static DoubleMat44 rotationMatrix(const QQuaternion &rot)
    {
        DoubleMat44 rotMat;
        double xp = double(rot.x());
        double yp = double(rot.y());
        double zp = double(rot.z());
        double wp = double(rot.scalar());

        const double f2x = xp + xp;
        const double f2y = yp + yp;
        const double f2z = zp + zp;
        const double f2xw = f2x * wp;
        const double f2yw = f2y * wp;
        const double f2zw = f2z * wp;
        const double f2xx = f2x * xp;
        const double f2xy = f2x * yp;
        const double f2xz = f2x * zp;
        const double f2yy = f2y * yp;
        const double f2yz = f2y * zp;
        const double f2zz = f2z * zp;

        rotMat.m[0][0] = 1. - (f2yy + f2zz);
        rotMat.m[1][0] =       f2xy - f2zw;
        rotMat.m[2][0] =       f2xz + f2yw;
        rotMat.m[0][1] =       f2xy + f2zw;
        rotMat.m[1][1] = 1. - (f2xx + f2zz);
        rotMat.m[2][1] =       f2yz - f2xw;
        rotMat.m[0][2] =       f2xz - f2yw;
        rotMat.m[1][2] =       f2yz + f2xw;
        rotMat.m[2][2] = 1. - (f2xx + f2yy);

        return rotMat;
    }

    double m[4][4];
};

static DoubleMat44 calcLocalTransform(const QQuick3DNode *node)
{
    DoubleVec3D pivotD(-node->pivot());
    const DoubleVec3D scaleD(node->scale());
    const DoubleVec3D posD(node->position());
    pivotD = pivotD * scaleD;
    DoubleMat44 localTransform;

    localTransform.m[0][0] = scaleD.x;
    localTransform.m[1][1] = scaleD.y;
    localTransform.m[2][2] = scaleD.z;

    localTransform.m[3][0] = pivotD.x;
    localTransform.m[3][1] = pivotD.y;
    localTransform.m[3][2] = pivotD.z;

    DoubleMat44 rotMat = DoubleMat44::rotationMatrix(node->rotation());
    localTransform = DoubleMat44::matrixMultiD(rotMat, localTransform);

    localTransform.m[3][0] += posD.x;
    localTransform.m[3][1] += posD.y;
    localTransform.m[3][2] += posD.z;

    return localTransform;
}

static DoubleMat44 calcGlobalTransform(const QQuick3DNode *node)
{
    DoubleMat44 localTrans = calcLocalTransform(node);
    QQuick3DNode *parent = node->parentNode();
    if (parent) {
        DoubleMat44 globalTrans = calcGlobalTransform(parent);
        return DoubleMat44::matrixMultiD(globalTrans, localTrans);
    }
    return localTrans;
}

static DoubleVec3D getNormalD(const DoubleMat44 &m)
{
    return DoubleVec3D(m.m[2][0], m.m[2][1], m.m[2][2]).normalized();
}

MouseArea3D *MouseArea3D::s_mouseGrab = nullptr;

MouseArea3D::MouseArea3D(QQuick3DNode *parent)
    : QQuick3DNode(parent)
{
}

QQuick3DViewport *MouseArea3D::view3D() const
{
    return m_view3D;
}

bool MouseArea3D::hovering() const
{
    return m_hovering;
}

bool MouseArea3D::dragging() const
{
    return m_dragging;
}

bool MouseArea3D::grabsMouse() const
{
    return m_grabsMouse;
}

bool MouseArea3D::active() const
{
    return m_active;
}

QPointF MouseArea3D::circlePickArea() const
{
    return m_circlePickArea;
}

qreal MouseArea3D::minAngle() const
{
    return m_minAngle;
}

QQuick3DNode *MouseArea3D::pickNode() const
{
    return m_pickNode;
}

MouseArea3D *MouseArea3D::dragHelper() const
{
    return m_dragHelper;
}

qreal MouseArea3D::x() const
{
    return m_x;
}

qreal MouseArea3D::y() const
{
    return m_y;
}

qreal MouseArea3D::width() const
{
    return m_width;
}

qreal MouseArea3D::height() const
{
    return m_height;
}

int MouseArea3D::priority() const
{
    return m_priority;
}

void MouseArea3D::setView3D(QQuick3DViewport *view3D)
{
    if (m_view3D == view3D)
        return;

    m_view3D = view3D;
    emit view3DChanged();
}

void MouseArea3D::setGrabsMouse(bool grabsMouse)
{
    if (m_grabsMouse == grabsMouse)
        return;

    m_grabsMouse = grabsMouse;

    if (!m_grabsMouse && s_mouseGrab == this) {
        setDragging(false);
        setHovering(false);
        s_mouseGrab = nullptr;
    }

    emit grabsMouseChanged();
}

void MouseArea3D::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;

    if (!m_active && s_mouseGrab == this) {
        setDragging(false);
        setHovering(false);
        s_mouseGrab = nullptr;
    }

    emit activeChanged();
}

void MouseArea3D::setCirclePickArea(const QPointF &pickArea)
{
    if (m_circlePickArea == pickArea)
        return;

    m_circlePickArea = pickArea;
    emit circlePickAreaChanged();
}

// This is the minimum angle for circle picking. At lower angles we fall back to picking on pickNode
void MouseArea3D::setMinAngle(qreal angle)
{
    if (qFuzzyCompare(m_minAngle, angle))
        return;

    m_minAngle = angle;
    emit minAngleChanged();
}

// This is the fallback pick node when circle picking can't be done due to low angle
// Pick node can't be used except in low angles, as long as only bounding box picking is supported
void MouseArea3D::setPickNode(QQuick3DNode *node)
{
    if (m_pickNode == node)
        return;

    m_pickNode = node;
    emit pickNodeChanged();
}

void MouseArea3D::setDragHelper(MouseArea3D *dragHelper)
{
    if (m_dragHelper == dragHelper)
        return;

    m_dragHelper = dragHelper;
    emit dragHelperChanged();
}

void MouseArea3D::setX(qreal x)
{
    if (qFuzzyCompare(m_x, x))
        return;

    m_x = x;
    emit xChanged();
}

void MouseArea3D::setY(qreal y)
{
    if (qFuzzyCompare(m_y, y))
        return;

    m_y = y;
    emit yChanged();
}

void MouseArea3D::setWidth(qreal width)
{
    if (qFuzzyCompare(m_width, width))
        return;

    m_width = width;
    emit widthChanged();
}

void MouseArea3D::setHeight(qreal height)
{
    if (qFuzzyCompare(m_height, height))
        return;

    m_height = height;
    emit heightChanged();
}

void MouseArea3D::setPriority(int level)
{
    if (m_priority == level)
        return;

    m_priority = level;
    emit priorityChanged();
}

void MouseArea3D::componentComplete()
{
    if (!m_view3D) {
        qmlDebug(this) << "property 'view3D' is not set!";
        return;
    }
    m_view3D->setAcceptedMouseButtons(Qt::LeftButton);
    m_view3D->setAcceptHoverEvents(true);
    m_view3D->setAcceptTouchEvents(false);
    m_view3D->installEventFilter(this);
}

static DoubleVec3D rayIntersectsPlaneD(const DoubleVec3D &rayPos0,
                                       const DoubleVec3D &rayPos1,
                                       const DoubleVec3D &planePos,
                                       const DoubleVec3D &planeNormal)
{
    const DoubleVec3D rayDirection = rayPos1 - rayPos0;
    const DoubleVec3D rayPos0RelativeToPlane = rayPos0 - planePos;

    const double dotPlaneRayDirection = DoubleVec3D::dotProduct(planeNormal, rayDirection);
    const double dotPlaneRayPos0 = -DoubleVec3D::dotProduct(planeNormal, rayPos0RelativeToPlane);

    if (qFuzzyIsNull(dotPlaneRayDirection)) {
        // The ray is is parallel to the plane. Note that if dotPlaneRayPos0 == 0, it
        // additionally means that the line lies in plane as well. In any case, we
        // signal that we cannot find a single intersection point.
        return DoubleVec3D(0., 0., -1.);
    }

    // Since we work with a ray (that has a start), distanceFromRayPos0ToPlane
    // must be above 0. If it was a line segment (with an end), it also need to be less than 1.
    // (Note: a third option would be a "line", which is different from a ray or segment in that
    // it has neither a start, nor an end). Then we wouldn't need to check the distance at all.
    // But that would also mean that the line could intersect the plane behind the camera, if
    // the line were directed away from the plane when looking forward.
    const double distanceFromRayPos0ToPlane = dotPlaneRayPos0 / dotPlaneRayDirection;
    if (distanceFromRayPos0ToPlane <= 0.)
        return DoubleVec3D(0., 0., -1.);
    return (rayPos0 + distanceFromRayPos0ToPlane * rayDirection);
}

QVector3D MouseArea3D::rayIntersectsPlane(const QVector3D &rayPos0,
                                          const QVector3D &rayPos1,
                                          const QVector3D &planePos,
                                          const QVector3D &planeNormal) const
{
    const DoubleVec3D rayPos0D(rayPos0);
    const DoubleVec3D rayPos1D(rayPos1);
    const DoubleVec3D planePosD(planePos);
    const DoubleVec3D planeNormalD(planeNormal);

    return rayIntersectsPlaneD(rayPos0D, rayPos1D, planePosD, planeNormalD).toVec3();
}

// Get a new scale based on a relative scene distance along a drag axes.
// relativeDistance contains x and y distance on drag plane. axisX indicates which axis corresponds
// to x value of relative distance. Similarly to axisY.
QVector3D MouseArea3D::getNewScale(const QVector3D &startScale, const QVector2D &relativeDistance,
                                   const QVector3D &axisX, const QVector3D &axisY)
{
    // Distance factor was calculated based on scale rod size.
    // For other use cases, relativeDistance needs to be adjusted to the same scale prior calling
    // this method.
    const float distanceFactor = 0.1f;
    QVector3D scaleVec(1.f, 1.f, 1.f);
    QVector3D xScaler(1.f, 1.f, 1.f);
    QVector3D yScaler(1.f, 1.f, 1.f);
    xScaler += axisX * relativeDistance.x() * distanceFactor;
    yScaler += axisY * relativeDistance.y() * distanceFactor;
    scaleVec *= xScaler;
    scaleVec *= yScaler;

    if (s_generalHelper)
        scaleVec = s_generalHelper->adjustScaleForSnap(scaleVec);

    return startScale * scaleVec;
}

qreal QmlDesigner::Internal::MouseArea3D::getNewRotationAngle(
        QQuick3DNode *node, const QVector3D &pressPos, const QVector3D &currentPos,
        const QVector3D &nodePos, qreal prevAngle, bool trackBall)
{
    const QVector3D dragVector = currentPos - pressPos;

    if (dragVector.length() < 0.001f)
        return prevAngle;

    // Get camera to node direction
    QVector3D cameraToNodeDir = getCameraToNodeDir(node);
    if (trackBall) {
        // Only the distance in plane direction is relevant in trackball drag
        QVector3D dragDir = QVector3D::crossProduct(getNormal(), cameraToNodeDir).normalized();
        QVector3D scenePos = pivotScenePosition(node);
        QVector3D screenDragDir = m_view3D->mapFrom3DScene(scenePos + dragDir);
        screenDragDir.setZ(0);
        dragDir = (screenDragDir - nodePos).normalized();
        const QVector3D pressToCurrent = (currentPos - pressPos);
        float magnitude = QVector3D::dotProduct(pressToCurrent, dragDir);
        qreal angle = -mouseDragMultiplier() * qreal(magnitude);
        return angle;
    } else {
        const QVector3D nodeToPress = (pressPos - nodePos).normalized();
        const QVector3D nodeToCurrent = (currentPos - nodePos).normalized();
        qreal angle = qAcos(qreal(QVector3D::dotProduct(nodeToPress, nodeToCurrent)));

        // Determine drag direction left/right
        QVector3D dragNormal = QVector3D::crossProduct(nodeToPress, nodeToCurrent).normalized();
        angle *= QVector3D::dotProduct(QVector3D(0.f, 0.f, 1.f), dragNormal) < 0 ? -1.0 : 1.0;

        // Determine drag ring orientation relative to camera
        angle *= QVector3D::dotProduct(getNormal(), cameraToNodeDir) < 0 ? -1.0 : 1.0;

        qreal adjustedPrevAngle = prevAngle;
        const qreal PI_2 = M_PI * 2.0;
        while (adjustedPrevAngle < -PI_2)
            adjustedPrevAngle += PI_2;
        while (adjustedPrevAngle > PI_2)
            adjustedPrevAngle -= PI_2;

        // at M_PI rotation, the angle flips to negative
        if (qAbs(angle - adjustedPrevAngle) > M_PI) {
            if (angle > adjustedPrevAngle)
                return prevAngle - (PI_2 - angle + adjustedPrevAngle);
            else
                return prevAngle + (PI_2 + angle - adjustedPrevAngle);
        } else {
            return prevAngle + angle - adjustedPrevAngle;
        }
    }

}

void QmlDesigner::Internal::MouseArea3D::applyRotationAngleToNode(
        QQuick3DNode *node, const QVector3D &startRotation, qreal angle)
{
    node->setEulerRotation(startRotation);
    if (!qFuzzyIsNull(angle)) {
        QVector3D normal = getNormal();
        node->rotate(qRadiansToDegrees(angle), normal, QQuick3DNode::SceneSpace);
    }
}

void MouseArea3D::applyFreeRotation(QQuick3DNode *node, const QVector3D &startRotation,
                                    const QVector3D &pressPos, const QVector3D &currentPos)
{
    QVector3D dragVector = currentPos - pressPos;

    if (dragVector.length() < 0.001f)
        return;

    const auto &transform = sceneTransform();
    const float *dataPtr(transform.data());
    QVector3D xAxis = QVector3D(dataPtr[0], dataPtr[1], dataPtr[2]).normalized();
    QVector3D yAxis = QVector3D(dataPtr[4], dataPtr[5], dataPtr[6]).normalized();
    QVector3D finalAxis = (dragVector.x() * yAxis + dragVector.y() * xAxis);

    qreal radians = qreal(finalAxis.length()) * mouseDragMultiplier();
    if (s_generalHelper)
        radians = s_generalHelper->adjustRotationForSnap(radians);

    finalAxis.normalize();

    node->setEulerRotation(startRotation);
    node->rotate(qRadiansToDegrees(radians), finalAxis, QQuick3DNode::SceneSpace);
}

// Calculate scene position of the node's pivot point, which in practice is just the position
// of the node without applying the pivot offset.
QVector3D MouseArea3D::pivotScenePosition(QQuick3DNode *node) const
{
    if (!node)
        return {};

    QQuick3DNode *parent = node->parentNode();
    if (!parent)
        return node->position();

    QMatrix4x4 localTransform;
    localTransform.translate(node->position());

    const QMatrix4x4 m = parent->sceneTransform() * localTransform;
    return QVector3D(m(0, 3), m(1, 3), m(2, 3));
}

double MouseArea3D::getRelativeScale(QQuick3DNode *node) const
{
    // Calculate the distance independent scale by first mapping the target's position to
    // the view. We then measure up a distance on the view (100px) that we use as an
    // "anchor" distance. Map the two positions back to the target node, and measure the
    // distance between them now, in the 3D scene. The difference between the two distances,
    // view and scene, will tell us what the distance independent scale should be.

    QVector3D nodePos(node->scenePosition());

    DoubleVec3D posInView1(m_view3D->mapFrom3DScene(nodePos));

    DoubleVec3D posInView2 = posInView1;
    posInView2.x = posInView2.x + 100.;

    DoubleVec3D scenePos1(m_view3D->mapTo3DScene(posInView1.toVec3()));
    DoubleVec3D scenePos2(m_view3D->mapTo3DScene(posInView2.toVec3()));

    return (scenePos1 - scenePos2).length() / 100.;
}

void MouseArea3D::forcePressEvent(double x, double y)
{
    m_forceCaptureNextPress = true;

    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers mods;
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(x, y), Qt::LeftButton, buttons, mods);
    eventFilter(m_view3D, &event);
}

void QmlDesigner::Internal::MouseArea3D::forceMoveEvent(double x, double y)
{
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers mods;
    QMouseEvent event(QEvent::MouseMove, QPointF(x, y), Qt::LeftButton, buttons, mods);
    eventFilter(m_view3D, &event);
}

void QmlDesigner::Internal::MouseArea3D::forceReleaseEvent(double x, double y)
{
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers mods;
    QMouseEvent event(QEvent::MouseButtonRelease, QPointF(x, y), Qt::LeftButton, buttons, mods);
    eventFilter(m_view3D, &event);
}

QVector3D MouseArea3D::getMousePosInPlane(const MouseArea3D *helper,
                                          const QPointF &mousePosInView) const
{
    if (!helper)
        helper = this;

    const DoubleVec3D mousePos1(float(mousePosInView.x()), float(mousePosInView.y()), 0);
    const DoubleVec3D rayPos0 = m_view3D->mapTo3DScene(mousePos1.toVec3());
    DoubleVec3D rayPos1;
    if (qobject_cast<QQuick3DOrthographicCamera *>(m_view3D->camera())) {
        if (auto cameraNode = static_cast<QSSGRenderCamera *>(QQuick3DObjectPrivate::get(m_view3D->camera())->spatialNode))
            rayPos1 = rayPos0 - rayPos0.length() * DoubleVec3D(cameraNode->getDirection());
    } else {
        DoubleVec3D dir;
        DoubleVec3D camPos = m_view3D->camera()->scenePosition();
        dir = (rayPos0 - camPos).normalized();
        rayPos1 = rayPos0 + rayPos0.length() * dir;
    }

    const DoubleVec3D globalPlanePosition = helper->mapPositionToScene(QVector3D(0, 0, 0));
    DoubleMat44 sceneTrans = calcGlobalTransform(helper);
    const DoubleVec3D intersectGlobalPos = rayIntersectsPlaneD(rayPos0, rayPos1,
                                                               globalPlanePosition,
                                                               -getNormalD(sceneTrans));

    if (qFuzzyCompare(intersectGlobalPos.z, -1.))
        return intersectGlobalPos.toVec3();

    return sceneTrans.inverted().transform(intersectGlobalPos).toVec3();
}

void QmlDesigner::Internal::MouseArea3D::setGeneralHelper(GeneralHelper *helper)
{
    s_generalHelper = helper;
}

static QPoint getPosFromMoveEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove:
        return static_cast<QMouseEvent *>(event)->pos();
    case QEvent::HoverMove:
        return static_cast<QHoverEvent *>(event)->pos();
    default:
        break;
    }
    return {};
}

bool MouseArea3D::eventFilter(QObject *, QEvent *event)
{
    if (!m_active || (m_grabsMouse && s_mouseGrab && s_mouseGrab != this
            && (m_priority <= s_mouseGrab->m_priority || s_mouseGrab->m_dragging
                || s_mouseGrab->m_forceCaptureNextPress))) {
        return false;
    }

    qreal pickAngle = 0.;

    auto mouseOnTopOfMouseArea = [this, &pickAngle](
            const QVector3D &mousePosInPlane, const QPointF &mousePos) -> bool {
        const bool onPlane = !qFuzzyCompare(mousePosInPlane.z(), -1)
                && mousePosInPlane.x() >= float(m_x)
                && mousePosInPlane.x() <= float(m_x + m_width)
                && mousePosInPlane.y() >= float(m_y)
                && mousePosInPlane.y() <= float(m_y + m_height);

        bool onCircle = true;
        bool pickSuccess = false;
        if (!qFuzzyIsNull(m_circlePickArea.y()) || !qFuzzyIsNull(m_minAngle)) {

            QVector3D cameraToMouseAreaDir = getCameraToNodeDir(this);
            const QVector3D mouseAreaDir = getNormal();
            qreal angle = qreal(QVector3D::dotProduct(cameraToMouseAreaDir, mouseAreaDir));
            // Do not allow selecting ring that is nearly perpendicular to camera, as dragging along
            // that plane would be difficult
            pickAngle = qAcos(angle);
            pickAngle = pickAngle > M_PI_2 ? pickAngle - M_PI_2 : M_PI_2 - pickAngle;
            if (pickAngle > m_minAngle) {
                if (!qFuzzyIsNull(m_circlePickArea.y())) {
                    qreal ringCenter = m_circlePickArea.x();
                    // Thickness is increased according to the angle to camera to keep projected
                    // circle thickness constant at all angles.
                    qreal divisor = qSin(pickAngle) * 2.; // This is never zero
                    qreal thickness = ((m_circlePickArea.y() / divisor));
                    qreal mousePosRadius = qSqrt(qreal(mousePosInPlane.x() * mousePosInPlane.x())
                                                 + qreal(mousePosInPlane.y() * mousePosInPlane.y()));
                    onCircle = ringCenter - thickness <= mousePosRadius
                            && ringCenter + thickness >= mousePosRadius;
                }
            } else {
                // Fall back to picking on the pickNode. At this angle, bounding box pick is not
                // a problem
                onCircle = false;
                if (m_pickNode) {
                    // With the introduction of global picking API,
                    // we need to pick all as various other geometries can often be the first
                    // pick result, such as camera frustum or light geometry
                    const QList<QQuick3DPickResult> results = m_view3D->pickAll(float(mousePos.x()),
                                                                                float(mousePos.y()));
                    for (const auto &pr : results) {
                        if (pr.objectHit() == m_pickNode) {
                            pickSuccess = true;
                            break;
                        }
                    }
                }
            }
        }
        return (onCircle && onPlane) || pickSuccess;
    };

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto const mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // Reset drag helper area to global transform of this area
            if (m_dragHelper) {
                m_dragHelper->setPosition(scenePosition());
                m_dragHelper->setRotation(sceneRotation());
                m_dragHelper->setScale(sceneScale());
            }
            m_mousePosInPlane = getMousePosInPlane(m_dragHelper, mouseEvent->pos());
            if (m_forceCaptureNextPress || mouseOnTopOfMouseArea(m_mousePosInPlane, mouseEvent->pos())) {
                m_forceCaptureNextPress = false;
                setDragging(true);
                emit pressed(m_mousePosInPlane.toVector2D(), mouseEvent->pos(), pickAngle);
                if (m_grabsMouse) {
                    if (s_mouseGrab && s_mouseGrab != this) {
                        s_mouseGrab->setDragging(false);
                        s_mouseGrab->setHovering(false);
                    }
                    s_mouseGrab = this;
                    setHovering(true);
                }
                event->accept();
                return true;
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto const mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (m_dragging) {
                QVector3D mousePosInPlane = getMousePosInPlane(m_dragHelper, mouseEvent->pos());
                if (qFuzzyCompare(mousePosInPlane.z(), -1))
                    mousePosInPlane = m_mousePosInPlane;
                setDragging(false);
                emit released(mousePosInPlane.toVector2D(), mouseEvent->pos());
                if (m_grabsMouse) {
                    if (s_mouseGrab && s_mouseGrab != this) {
                        s_mouseGrab->setDragging(false);
                        s_mouseGrab->setHovering(false);
                    }
                    if (mouseOnTopOfMouseArea(mousePosInPlane, mouseEvent->pos())) {
                        s_mouseGrab = this;
                        setHovering(true);
                    } else {
                        s_mouseGrab = nullptr;
                        setHovering(false);
                    }
                }
                event->accept();
                return true;
            }
        }
        break;
    }
    case QEvent::MouseMove:
    case QEvent::HoverMove: {
        const QPoint pos = getPosFromMoveEvent(event);
        const QVector3D mousePosInPlane = getMousePosInPlane(m_dragging ? m_dragHelper : this, pos);
        const bool hasMouse = mouseOnTopOfMouseArea(mousePosInPlane, pos);

        setHovering(hasMouse);

        if (m_grabsMouse) {
            if (m_hovering && s_mouseGrab && s_mouseGrab != this)
                s_mouseGrab->setHovering(false);

            if (m_hovering || m_dragging)
                s_mouseGrab = this;
            else if (s_mouseGrab == this)
                s_mouseGrab = nullptr;
        }

        if (m_dragging && (m_circlePickArea.y() > 0. || !qFuzzyCompare(mousePosInPlane.z(), -1))) {
            m_mousePosInPlane = mousePosInPlane;
            emit dragged(mousePosInPlane.toVector2D(), pos);
        }

        break;
    }
    default:
        break;
    }

    return false;
}

void MouseArea3D::setDragging(bool enable)
{
    if (m_dragging == enable)
        return;

    m_dragging = enable;
    emit draggingChanged();
}

void MouseArea3D::setHovering(bool enable)
{
    if (m_hovering == enable)
        return;

    m_hovering = enable;
    emit hoveringChanged();
}

QVector3D MouseArea3D::getNormal() const
{
    const auto &transform = sceneTransform();
    const float *dataPtr(transform.data());
    return QVector3D(dataPtr[8], dataPtr[9], dataPtr[10]).normalized();
}

QVector3D MouseArea3D::getCameraToNodeDir(QQuick3DNode *node) const
{
    QVector3D dir;
    if (m_view3D->camera()) {
        // We need to do a cast here to be compatible with Qt 5.x.
        // From Qt 6.2 the type can be read from the node directly.
        if (qobject_cast<QQuick3DOrthographicCamera *>(m_view3D->camera())) {
            if (auto renderCamera = QQuick3DObjectPrivate::get(m_view3D->camera())->spatialNode)
                dir -= static_cast<QSSGRenderCamera *>(renderCamera)->getDirection();
        } else {
            QVector3D camPos = m_view3D->camera()->scenePosition();
            QVector3D nodePos = pivotScenePosition(node);
            dir = (nodePos - camPos).normalized();
        }
    }
    return dir;
}

}
}

#endif // QUICK3D_MODULE
