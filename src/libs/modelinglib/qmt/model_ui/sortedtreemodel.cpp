// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sortedtreemodel.h"

#include "qmt/model/massociation.h"
#include "qmt/model/mcanvasdiagram.h"
#include "qmt/model/mclass.h"
#include "qmt/model/mcomponent.h"
#include "qmt/model/mconnection.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/minheritance.h"
#include "qmt/model/mitem.h"
#include "qmt/model/mpackage.h"
#include "qmt/model/mconstvisitor.h"
#include "treemodel.h"

namespace qmt {

namespace {

class Filter : public MConstVisitor {
public:

    void setModelTreeViewData(const ModelTreeViewData *viewData)
    {
        m_viewData = viewData;
    }

    void setModelTreeFilterData(const ModelTreeFilterData *filterData)
    {
        m_filterData = filterData;
    }

    bool keep() const { return m_keep; }

    void visitMElement(const MElement *element) override
    {
        if (!m_filterData->stereotypes().isEmpty()) {
            const QStringList stereotypes = element->stereotypes();
            bool containsElementStereotype = std::any_of(
                stereotypes.constBegin(), stereotypes.constEnd(),
                [&](const QString &s) { return m_filterData->stereotypes().contains(s); });
            if (!containsElementStereotype) {
                m_keep = false;
                return;
            }
        }
    }

    void visitMObject(const MObject *object) override
    {
        if (!m_filterData->name().isEmpty() && m_filterData->name() != object->name())
            m_keep = false;
        else
            visitMElement(object);
    }

    void visitMPackage(const MPackage *package) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Package)
        {
            m_keep = false;
        } else {
            visitMObject(package);
        }
    }

    void visitMClass(const MClass *klass) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Class)
        {
            m_keep = false;
        } else {
            visitMObject(klass);
        }
    }

    void visitMComponent(const MComponent *component) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Component)
        {
            m_keep = false;
        } else {
            visitMObject(component);
        }
    }

    void visitMDiagram(const MDiagram *diagram) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Diagram)
        {
            m_keep = false;
        } else {
            visitMObject(diagram);
        }
    }

    void visitMCanvasDiagram(const MCanvasDiagram *diagram) override
    {
        visitMDiagram(diagram);
    }

    void visitMItem(const MItem *item) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Item)
        {
            m_keep = false;
        } else {
            visitMObject(item);
        }
    }

    void visitMRelation(const MRelation *relation) override
    {
        if (!m_viewData->showRelations())
            m_keep = false;
        else if (!m_filterData->name().isEmpty() && m_filterData->name() != relation->name())
            m_keep = false;
        else
            visitMElement(relation);
    }

    void visitMDependency(const MDependency *dependency) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Dependency)
        {
            m_keep = false;
        } else {
            switch (m_filterData->direction()) {
            case ModelTreeFilterData::Direction::Any:
                break;
            case ModelTreeFilterData::Direction::Outgoing:
                if (dependency->direction() != MDependency::AToB)
                    m_keep = false;
                break;
            case ModelTreeFilterData::Direction::Incoming:
                if (dependency->direction() != MDependency::BToA)
                    m_keep = false;
                break;
            case ModelTreeFilterData::Direction::Bidirectional:
                if (dependency->direction() != MDependency::Bidirectional)
                    m_keep = false;
                break;
            }
            if (m_keep)
                visitMRelation(dependency);
        }
    }

    void visitMInheritance(const MInheritance *inheritance) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Inheritance)
        {
            m_keep = false;
        } else {
            visitMRelation(inheritance);
        }
    }

    void visitMAssociation(const MAssociation *association) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Association)
        {
            m_keep = false;
        } else {
            visitMRelation(association);
        }
    }

    void visitMConnection(const MConnection *connection) override
    {
        if (m_filterData->type() != ModelTreeFilterData::Type::Any
            && m_filterData->type() != ModelTreeFilterData::Type::Connection)
        {
            m_keep = false;
        } else {
            visitMRelation(connection);
        }
    }

private:
    const ModelTreeViewData *m_viewData = nullptr;
    const ModelTreeFilterData *m_filterData = nullptr;
    bool m_keep = true;
};

}

SortedTreeModel::SortedTreeModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
    setRecursiveFilteringEnabled(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);

    m_delayedSortTimer.setSingleShot(true);
    connect(&m_delayedSortTimer, &QTimer::timeout, this, &SortedTreeModel::onDelayedSortTimeout);

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &SortedTreeModel::onTreeModelRowsInserted);
    connect(this, &QAbstractItemModel::dataChanged,
            this, &SortedTreeModel::onDataChanged);
}

SortedTreeModel::~SortedTreeModel()
{
}

void SortedTreeModel::setTreeModel(TreeModel *treeModel)
{
    m_treeModel = treeModel;
    setSourceModel(treeModel);
    startDelayedSortTimer();
}

void SortedTreeModel::setModelTreeViewData(const ModelTreeViewData &viewData)
{
    m_modelTreeViewData = viewData;
    beginResetModel();
    endResetModel();
    startDelayedSortTimer();
}

void SortedTreeModel::setModelTreeFilterData(const ModelTreeFilterData &filterData)
{
    m_modelTreeViewFilterData = filterData;
    beginResetModel();
    endResetModel();
    startDelayedSortTimer();
}

bool SortedTreeModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    MElement *element = m_treeModel->element(index);
    if (element) {
        Filter filter;
        filter.setModelTreeViewData(&m_modelTreeViewData);
        filter.setModelTreeFilterData(&m_modelTreeViewFilterData);
        element->accept(&filter);
        if (!filter.keep())
            return false;
    }
    return true;
}

bool SortedTreeModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    TreeModel::ItemType leftItemType = TreeModel::ItemType(sourceModel()->data(left, TreeModel::RoleItemType).toInt());
    TreeModel::ItemType rightItemType = TreeModel::ItemType(sourceModel()->data(right, TreeModel::RoleItemType).toInt());
    if (leftItemType < rightItemType) {
        return true;
    } else if (leftItemType > rightItemType) {
        return false;
    } else {
        QVariant l = sourceModel()->data(left);
        QVariant r = sourceModel()->data(right);
        switch (sortCaseSensitivity()) {
        case Qt::CaseInsensitive:
            return l.toString().compare(r.toString(), Qt::CaseInsensitive) < 0;
        case Qt::CaseSensitive:
            return l.toString() < r.toString();
        default:
            return l.toString() < r.toString();
        }
    }
}

void SortedTreeModel::onTreeModelRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    startDelayedSortTimer();
}

void SortedTreeModel::onDataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)
{
    startDelayedSortTimer();
}

void SortedTreeModel::onDelayedSortTimeout()
{
    sort(0, Qt::AscendingOrder);
}

void SortedTreeModel::startDelayedSortTimer()
{
    m_delayedSortTimer.start(1000);
}

} // namespace qmt
