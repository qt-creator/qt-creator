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

#include "shapeprovider.h"

#include <QVector>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class SCShapeProvider : public ShapeProvider
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
