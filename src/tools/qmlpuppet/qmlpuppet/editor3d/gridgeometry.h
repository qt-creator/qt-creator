// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

namespace QmlDesigner {
namespace Internal {

class GridGeometry : public GeometryBase
{
    Q_OBJECT

    Q_PROPERTY(int lines READ lines WRITE setLines NOTIFY linesChanged)
    Q_PROPERTY(float step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(bool isCenterLine READ isCenterLine WRITE setIsCenterLine NOTIFY isCenterLineChanged)
    Q_PROPERTY(bool isSubdivision MEMBER m_isSubdivision)

public:
    GridGeometry();
    ~GridGeometry() override;

    int lines() const;
    float step() const;
    bool isCenterLine() const;

public slots:
    void setLines(int count);
    void setStep(float step);
    void setIsCenterLine(bool enabled);

signals:
    void linesChanged();
    void stepChanged();
    void isCenterLineChanged();

protected:
    void doUpdateGeometry() override;
#if QT_VERSION_MAJOR == 6 && QT_VERSION_MINOR == 4
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;
#endif

private:
    void fillVertexData(QByteArray &vertexData);

    int m_lines = 20; // number of lines on 1 side of an axis (so total number of grid lines in 1 direction = 2 * m_lines + 1)
    float m_step = .1f;
    bool m_isCenterLine = false;
    bool m_isSubdivision = false;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::GridGeometry)

#endif // QUICK3D_MODULE
