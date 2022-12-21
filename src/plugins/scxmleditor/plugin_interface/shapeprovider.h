// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmltag.h"
#include <QByteArray>
#include <QIcon>
#include <QObject>
#include <QString>

namespace ScxmlEditor {

namespace PluginInterface {

class ShapeProvider : public QObject
{
    Q_OBJECT

public:
    struct Shape
    {
        QString title;
        QIcon icon;
        QStringList filters;
        QByteArray scxmlData;
        QVariant userData;
    };

    struct ShapeGroup
    {
        ~ShapeGroup()
        {
            qDeleteAll(shapes);
            shapes.clear();
        }

        QString title;
        QVector<Shape*> shapes;
        void addShape(Shape *shape)
        {
            shapes << shape;
        }
    };

    explicit ShapeProvider(QObject *parent = nullptr);

    virtual int groupCount() const = 0;
    virtual QString groupTitle(int groupIndex) const = 0;

    virtual int shapeCount(int groupIndex) const = 0;
    virtual QString shapeTitle(int groupIndex, int shapeIndex) const = 0;
    virtual QIcon shapeIcon(int groupIndex, int shapeIndex) const = 0;

    virtual bool canDrop(int groupIndex, int shapeIndex, ScxmlTag *parent) const = 0;
    virtual QByteArray scxmlCode(int groupIndex, int shapeIndex, ScxmlTag *parent) const = 0;

signals:
    void changed();
};

} // namespace PluginInterface
} // namespace ScxmlEditor
