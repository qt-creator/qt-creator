// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scshapeprovider.h"
#include "scxmleditortr.h"
#include "scxmleditoricons.h"
#include "scxmltag.h"

#include <QDebug>

using namespace ScxmlEditor::PluginInterface;

SCShapeProvider::SCShapeProvider(QObject *parent)
    : ShapeProvider(parent)
{
    init();
}

SCShapeProvider::~SCShapeProvider()
{
    clear();
}

void SCShapeProvider::clear()
{
    qDeleteAll(m_groups);
    m_groups.clear();
}

void SCShapeProvider::initGroups()
{
    init();
}

void SCShapeProvider::init()
{
    ShapeGroup *group = addGroup(Tr::tr("Common States"));
    group->addShape(createShape(Tr::tr("Initial"), Icons::INITIAL.icon(),
                                {"scxml", "state", "parallel"}, "<initial/>"));
    group->addShape(createShape(Tr::tr("Final"), Icons::FINAL.icon(),
                                {"scxml", "state", "parallel"}, "<final/>"));
    group->addShape(createShape(Tr::tr("State"), Icons::STATE.icon(),
                                {"scxml", "state", "parallel"}, "<state/>"));
    group->addShape(createShape(Tr::tr("Parallel"), Icons::PARALLEL.icon(),
                                {"scxml", "state", "parallel"}, "<parallel/>"));
    group->addShape(createShape(Tr::tr("History"), Icons::HISTORY.icon(),
                                {"state", "parallel"}, "<history/>"));
}

ShapeProvider::Shape *SCShapeProvider::shape(int groupIndex, int shapeIndex)
{
    if (groupIndex >= 0 && groupIndex < m_groups.count()) {
        if (shapeIndex >= 0 && shapeIndex < m_groups[groupIndex]->shapes.count())
            return m_groups[groupIndex]->shapes[shapeIndex];
    }

    return nullptr;
}

ShapeProvider::ShapeGroup *SCShapeProvider::group(int groupIndex)
{
    if (groupIndex >= 0 && groupIndex < m_groups.count())
        return m_groups[groupIndex];

    return nullptr;
}

SCShapeProvider::ShapeGroup *SCShapeProvider::addGroup(const QString &title)
{
    auto group = new ShapeGroup;
    group->title = title;
    m_groups << group;
    return group;
}

SCShapeProvider::Shape *SCShapeProvider::createShape(const QString &title, const QIcon &icon, const QStringList &filters, const QByteArray &scxmlData, const QVariant &userData)
{
    auto shape = new Shape;
    shape->title = title;
    shape->icon = icon;
    shape->filters = filters;
    shape->scxmlData = scxmlData;
    shape->userData = userData;

    return shape;
}

int SCShapeProvider::groupCount() const
{
    return m_groups.count();
}

QString SCShapeProvider::groupTitle(int groupIndex) const
{
    if (groupIndex >= 0 && groupIndex < m_groups.count())
        return m_groups[groupIndex]->title;

    return QString();
}

int SCShapeProvider::shapeCount(int groupIndex) const
{
    if (groupIndex >= 0 && groupIndex < m_groups.count())
        return m_groups[groupIndex]->shapes.count();

    return 0;
}

QString SCShapeProvider::shapeTitle(int groupIndex, int shapeIndex) const
{
    if (groupIndex >= 0 && groupIndex < m_groups.count()) {
        if (shapeIndex >= 0 && shapeIndex < m_groups[groupIndex]->shapes.count())
            return m_groups[groupIndex]->shapes[shapeIndex]->title;
    }

    return QString();
}

QIcon SCShapeProvider::shapeIcon(int groupIndex, int shapeIndex) const
{
    if (groupIndex >= 0 && groupIndex < m_groups.count()) {
        if (shapeIndex >= 0 && shapeIndex < m_groups[groupIndex]->shapes.count())
            return m_groups[groupIndex]->shapes[shapeIndex]->icon;
    }

    return QIcon();
}

bool SCShapeProvider::canDrop(int groupIndex, int shapeIndex, ScxmlTag *parent) const
{
    QString tagName = parent ? parent->tagName(false) : "scxml";
    if (groupIndex >= 0 && groupIndex < m_groups.count()) {
        if (shapeIndex >= 0 && shapeIndex < m_groups[groupIndex]->shapes.count()) {
            const QStringList &filters = m_groups[groupIndex]->shapes[shapeIndex]->filters;
            return filters.isEmpty() || filters.contains(tagName);
        }
    }

    return false;
}

QByteArray SCShapeProvider::scxmlCode(int groupIndex, int shapeIndex, ScxmlTag *parent) const
{
    Q_UNUSED(parent)

    if (groupIndex >= 0 && groupIndex < m_groups.count()) {
        if (shapeIndex >= 0 && shapeIndex < m_groups[groupIndex]->shapes.count())
            return m_groups[groupIndex]->shapes[shapeIndex]->scxmlData;
    }

    return QByteArray();
}
