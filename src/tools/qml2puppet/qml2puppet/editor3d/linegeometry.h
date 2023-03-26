// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef QUICK3D_MODULE

#include "geometrybase.h"

#include <QtGui/QVector3D>

namespace QmlDesigner {
namespace Internal {

class LineGeometry : public GeometryBase
{
    Q_OBJECT
    Q_PROPERTY(QVector3D startPos READ startPos WRITE setStartPos NOTIFY startPosChanged)
    Q_PROPERTY(QVector3D endPos READ endPos WRITE setEndPos NOTIFY endPosChanged)

public:
    LineGeometry();
    ~LineGeometry() override;

    QVector3D startPos() const;
    QVector3D endPos() const;

public slots:
    void setStartPos(const QVector3D &pos);
    void setEndPos(const QVector3D &pos);

signals:
    void startPosChanged();
    void endPosChanged();

protected:
    void doUpdateGeometry() override;

private:
    QVector3D m_startPos;
    QVector3D m_endPos;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::LineGeometry)

#endif // QUICK3D_MODULE
