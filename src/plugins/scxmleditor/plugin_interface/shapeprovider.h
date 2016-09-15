/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
