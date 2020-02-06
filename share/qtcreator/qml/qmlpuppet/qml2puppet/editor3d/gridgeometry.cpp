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

#ifdef QUICK3D_MODULE

#include "gridgeometry.h"

#include <QtQuick3DRuntimeRender/private/qssgrendergeometry_p.h>

namespace QmlDesigner {
namespace Internal {

GridGeometry::GridGeometry()
    : QQuick3DGeometry()
{
}

GridGeometry::~GridGeometry()
{
}

int GridGeometry::lines() const
{
    return m_lines;
}

float GridGeometry::step() const
{
    return m_step;
}

bool GridGeometry::isCenterLine() const
{
    return m_isCenterLine;
}

// Number of lines on each side of the center lines.
// These lines are not drawn if m_isCenterLine is true; lines and step are simply used to calculate
// the length of the center line in that case.
void GridGeometry::setLines(int count)
{
    count = qMax(count, 1);
    if (m_lines == count)
        return;
    m_lines = qMax(count, 1);
    emit linesChanged();
    update();
}

// Space between lines
void GridGeometry::setStep(float step)
{
    step = qMax(step, 0.0f);
    if (qFuzzyCompare(m_step, step))
        return;
    m_step = step;
    emit stepChanged();
    update();
}

void GridGeometry::setIsCenterLine(bool enabled)
{
    if (m_isCenterLine == enabled)
        return;

    m_isCenterLine = enabled;
    emit isCenterLineChanged();
    update();
}

QSSGRenderGraphObject *GridGeometry::updateSpatialNode(QSSGRenderGraphObject *node)
{
    node = QQuick3DGeometry::updateSpatialNode(node);
    QSSGRenderGeometry *geometry = static_cast<QSSGRenderGeometry *>(node);
    geometry->clear();

    QByteArray vertexData;
    fillVertexData(vertexData);

    geometry->addAttribute(QSSGRenderGeometry::Attribute::PositionSemantic, 0,
                           QSSGRenderGeometry::Attribute::ComponentType::F32Type);
    geometry->setStride(12);
    geometry->setVertexData(vertexData);
    geometry->setPrimitiveType(QSSGRenderGeometry::Lines);

    int lastIndex = (vertexData.size() - 1) / int(sizeof(QVector3D));
    auto vertexPtr = reinterpret_cast<QVector3D *>(vertexData.data());
    geometry->setBounds(QVector3D(vertexPtr[0][0], vertexPtr[0][1], 0.0),
                        QVector3D(vertexPtr[lastIndex][0], vertexPtr[lastIndex][1], 0.0));
    return node;
}

void GridGeometry::fillVertexData(QByteArray &vertexData)
{
    const int numSubdivs = 1; // number of subdivision lines (i.e. lines between main grid lines)
    const int vtxSize = int(sizeof(float)) * 3 * 2;
    const int size = m_isCenterLine ? vtxSize
                  : m_isSubdivision ? 4 * m_lines * vtxSize * numSubdivs
                                    : 4 * m_lines * vtxSize;
    vertexData.resize(size);
    float *dataPtr = reinterpret_cast<float *>(vertexData.data());

    float x0 = -float(m_lines) * m_step;
    float y0 = x0;
    float x1 = -x0;
    float y1 = x1;

    if (m_isCenterLine) {
        // start position
        dataPtr[0] = 0.f;
        dataPtr[1] = y0;
        dataPtr[2] = 0.f;
        // end position
        dataPtr[3] = 0.f;
        dataPtr[4] = y1;
        dataPtr[5] = 0.f;
    } else {
        // Lines are created so that bounding box can later be calculated from first and last vertex
        if (m_isSubdivision) {
            const float subdivStep = m_step / float(numSubdivs + 1);
            const int subdivMainLines = m_lines * 2;
            auto generateSubLines = [&](float x0, float y0, float x1, float y1, bool vertical) {
                for (int i = 0; i < subdivMainLines; ++i) {
                    for (int j = 1; j <= numSubdivs; ++j) {
                        // start position
                        dataPtr[0] = vertical ? x0 + i * m_step + j * subdivStep : x0;
                        dataPtr[1] = vertical ? y0 : y0 + i * m_step + j * subdivStep;
                        dataPtr[2] = .0f;
                        // end position
                        dataPtr[3] = vertical ? x0 + i * m_step + j * subdivStep : x1;
                        dataPtr[4] = vertical ? y1 : y0 + i * m_step + j * subdivStep;
                        dataPtr[5] = .0f;
                        dataPtr += 6;
                    }
                }
            };
            generateSubLines(x0, y0, x1, y1, true);
            generateSubLines(x0, y0, x1, y1, false);
        } else {
            auto generateLines = [this, &dataPtr](float x0, float y0, float x1, float y1, bool vertical) {
                for (int i = 0; i < m_lines; ++i) {
                    // start position
                    dataPtr[0] = vertical ? x0 + i * m_step : x0;
                    dataPtr[1] = vertical ? y0 : y0 + i * m_step;
                    dataPtr[2] = .0f;
                    // end position
                    dataPtr[3] = vertical ? x0 + i * m_step : x1;
                    dataPtr[4] = vertical ? y1 : y0 + i * m_step;
                    dataPtr[5] = .0f;
                    dataPtr += 6;
                }
            };
            generateLines(x0, y0, x1, y1, true);
            generateLines(x0, y0, x1, y1, false);
            generateLines(x0, m_step, x1, y1, false);
            generateLines(m_step, y0, x1, y1, true);
        }
    }
}

}
}

#endif // QUICK3D_MODULE
