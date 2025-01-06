// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

#include <QtQuick3D/private/qquick3dcamera_p.h>

namespace QmlDesigner {
namespace Internal {

class CameraGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QRectF viewPortRect READ viewPortRect WRITE setViewPortRect NOTIFY viewPortRectChanged)

public:
    CameraGeometry();
    ~CameraGeometry() override;

    QQuick3DCamera *camera() const;
    QRectF viewPortRect() const;

public slots:
    void setCamera(QQuick3DCamera *camera);
    void setViewPortRect(const QRectF &rect);
    void handleCameraPropertyChange();

signals:
    void cameraChanged();
    void viewPortRectChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
    void doUpdateGeometry() override;

private:
    void fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                        QVector3D &minBounds, QVector3D &maxBounds);

    QQuick3DCamera *m_camera = nullptr;
    QRectF m_viewPortRect;
    bool m_cameraUpdatePending = false;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::CameraGeometry)

#endif // QUICK3D_MODULE
