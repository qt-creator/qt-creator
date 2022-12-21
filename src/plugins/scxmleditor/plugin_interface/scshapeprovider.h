// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "shapeprovider.h"

#include <QVector>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class SCShapeProvider final : public ShapeProvider
{
    Q_OBJECT
public:
    SCShapeProvider(QObject *parent = nullptr);
    ~SCShapeProvider() override;

    int groupCount() const override;
    QString groupTitle(int groupIndex) const override;

    int shapeCount(int groupIndex) const override;
    QString shapeTitle(int groupIndex, int shapeIndex) const override;
    QIcon shapeIcon(int groupIndex, int shapeIndex) const override;

    bool canDrop(int groupIndex, int shapeIndex, ScxmlTag *parent) const override;
    QByteArray scxmlCode(int groupIndex, int shapeIndex, ScxmlTag *parent) const override;

protected:
    virtual void clear();
    virtual void initGroups();
    virtual ShapeGroup *addGroup(const QString &title);
    virtual Shape *createShape(const QString &title, const QIcon &icon, const QStringList &filters, const QByteArray &scxmlData, const QVariant &userData = QVariant());
    Shape *shape(int groupIndex, int shapeIndex);
    ShapeGroup *group(int groupIndex);

private:
    void init();
    QVector<ShapeGroup*> m_groups;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
