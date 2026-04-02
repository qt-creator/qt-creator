// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "shapeprovider.h"

#include <QList>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class SCShapeProvider final : public ShapeProvider
{
    Q_OBJECT
public:
    explicit SCShapeProvider(QObject *parent = nullptr);
    ~SCShapeProvider() final;

    int groupCount() const final;
    QString groupTitle(int groupIndex) const final;

    int shapeCount(int groupIndex) const final;
    QString shapeTitle(int groupIndex, int shapeIndex) const final;
    QIcon shapeIcon(int groupIndex, int shapeIndex) const final;

    bool canDrop(int groupIndex, int shapeIndex, ScxmlTag *parent) const final;
    QByteArray scxmlCode(int groupIndex, int shapeIndex, ScxmlTag *parent) const final;

protected:
    void clear();
    void initGroups();
    ShapeGroup *addGroup(const QString &title);
    Shape *createShape(
        const QString &title,
        const QIcon &icon,
        const QStringList &filters,
        const QByteArray &scxmlData,
        const QVariant &userData = QVariant());
    Shape *shape(int groupIndex, int shapeIndex);
    ShapeGroup *group(int groupIndex);

private:
    void init();
    QList<ShapeGroup*> m_groups;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
