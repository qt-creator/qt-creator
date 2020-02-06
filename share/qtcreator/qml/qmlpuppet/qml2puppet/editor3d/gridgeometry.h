/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#ifdef QUICK3D_MODULE

#include <QtQuick3D/private/qquick3dgeometry_p.h>

namespace QmlDesigner {
namespace Internal {

class GridGeometry : public QQuick3DGeometry
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

public Q_SLOTS:
    void setLines(int count);
    void setStep(float step);
    void setIsCenterLine(bool enabled);

Q_SIGNALS:
    void linesChanged();
    void stepChanged();
    void isCenterLineChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

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
