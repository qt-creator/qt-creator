// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "annotationlist.h"

#include <QPainter>
#include <QItemSelectionModel>

#include <variant>

namespace QmlDesigner {

// Entry

AnnotationListEntry::AnnotationListEntry(const ModelNode &modelnode)
    : id(modelnode.id())
    , annotationName(modelnode.customId())
    , annotation(modelnode.annotation())
    , node(modelnode)
{
}

AnnotationListEntry::AnnotationListEntry(const QString &argId, const QString &argAnnotationName,
                                         const Annotation &argAnnotation, const ModelNode &argNode)
    : id(argId)
    , annotationName(argAnnotationName)
    , annotation(argAnnotation)
    , node(argNode)
{
}


// Model

AnnotationListModel::AnnotationListModel(ModelNode rootNode, AnnotationListView *parent)
    : QAbstractItemModel(parent)
    , m_annotationView(parent)
    , m_rootNode(rootNode)
{
    fillModel();
}

void AnnotationListModel::setRootNode(ModelNode rootNode)
{
    m_rootNode = rootNode;
    resetModel();
}

void AnnotationListModel::resetModel()
{
    beginResetModel();

    m_annoList.clear();

    fillModel();

    endResetModel();
}

ModelNode AnnotationListModel::getModelNode(int id) const
{
    if (id >= 0 && id < int(m_annoList.size()))
        return m_annoList.at(id).node;
    return {};
}

AnnotationListEntry AnnotationListModel::getStoredAnnotation(int id) const
{
    if (id >= 0 && id < int(m_annoList.size()))
        return m_annoList.at(id);

    return {};
}

void AnnotationListModel::fillModel()
{
    if (m_rootNode.isValid()) {
        const QList<QmlDesigner::ModelNode> allNodes = m_rootNode.allSubModelNodesAndThisNode();

        for (const QmlDesigner::ModelNode &node : allNodes) {
            if (node.hasCustomId() || node.hasAnnotation()) {
                m_annoList.emplace_back(node);
            }
        }
    }
}

QModelIndex AnnotationListModel::index(int row,
                                       int column,
                                       [[maybe_unused]] const QModelIndex &parent) const
{
    return createIndex(row, column);
}

QModelIndex AnnotationListModel::parent(const QModelIndex &) const
{
    return {};
}

int AnnotationListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_annoList.size());
}

int AnnotationListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_numberOfColumns;
}

QVariant AnnotationListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_annoList.size()))
        return {};

    const auto &item = m_annoList.at(index.row());

    switch (role) {
    case AnnotationListModel::IdRow:
        return item.id;
    case AnnotationListModel::NameRow:
        return item.annotationName;
    case AnnotationListModel::AnnotationsCountRow:
        return item.annotation.commentsSize();
    default:
        return {};
    }
}

Qt::ItemFlags AnnotationListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_annoList.size()))
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void AnnotationListModel::storeChanges(int row, const QString &customId, const Annotation &annotation)
{
    if (row < 0 || row >= rowCount())
        return;

    auto &item = m_annoList[row];

    item.annotationName = customId;
    item.annotation = annotation;

    emit dataChanged(index(row, 1), index(row, 2));
}

void AnnotationListModel::saveChanges()
{
    for (auto &item : m_annoList) {
        if (ModelNode &node = item.node) {
            const QString &annoName = item.annotationName;
            const Annotation &anno = item.annotation;

            node.setCustomId(annoName);
            node.setAnnotation(anno);
        }
    }
}


// View

AnnotationListView::AnnotationListView(ModelNode rootNode, QWidget *parent)
    : Utils::ListView(parent)
    , m_model(new AnnotationListModel(rootNode, this))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

    setModel(m_model);
    setItemDelegate(new AnnotationListDelegate(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setActivationMode(Utils::SingleClickActivation);
    setSelectionRectVisible(true);
    setDragEnabled(false);
}

void AnnotationListView::setRootNode(ModelNode rootNode)
{
    m_model->setRootNode(rootNode);
}

ModelNode AnnotationListView::getModelNodeByAnnotationId(int id) const
{
    return m_model->getModelNode(id);
}

AnnotationListEntry AnnotationListView::getStoredAnnotationById(int id) const
{
    return m_model->getStoredAnnotation(id);
}

void AnnotationListView::selectRow(int row)
{
    if (m_model->rowCount() > row) {
        QModelIndex index = m_model->index(row, 0);
        setCurrentIndex(index);

        emit activated(index);
    }
}

int AnnotationListView::rowCount() const
{
    return m_model->rowCount();
}

void AnnotationListView::storeChangesInModel(int row, const QString &customId, const Annotation &annotation)
{
    m_model->storeChanges(row, customId, annotation);
}

void AnnotationListView::saveChangesFromModel()
{
    m_model->saveChanges();
}


// Delegate

AnnotationListDelegate::AnnotationListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize AnnotationListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QFontMetrics fm(option.font);
    QSize s;
    s.setWidth(option.rect.width());
    s.setHeight(fm.height() * 2 + 10);
    return s;
}

void AnnotationListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor backgroundColor;
    QColor textColor;

    bool selected = opt.state & QStyle::State_Selected;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
    } else {
        backgroundColor = opt.palette.window().color();
        painter->setBrush(backgroundColor);
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    // Set Text Color
    if (opt.state & QStyle::State_Selected)
        textColor = opt.palette.highlightedText().color();
    else
        textColor = opt.palette.text().color();

    painter->setPen(textColor);

    const QString itemId = index.data(AnnotationListModel::IdRow).toString();
    painter->drawText(6, 2 + opt.rect.top() + fm.ascent(), itemId);

    const QString annotationCounter = index.data(AnnotationListModel::AnnotationsCountRow).toString();
    painter->drawText(opt.rect.right() - fm.horizontalAdvance(annotationCounter) - 6,
                      2 + opt.rect.top() + fm.ascent(), annotationCounter);

    // Annotation name block:
    QColor mix;
    mix.setRgbF(0.7 * textColor.redF()   + 0.3 * backgroundColor.redF(),
                0.7 * textColor.greenF() + 0.3 * backgroundColor.greenF(),
                0.7 * textColor.blueF()  + 0.3 * backgroundColor.blueF());
    painter->setPen(mix);

    const QString annotationName = index.data(AnnotationListModel::NameRow).toString().trimmed();

    painter->drawText(6, opt.rect.top() + fm.ascent() + fm.height() + 6, annotationName);

    // Separator lines
    const QRectF innerRect = QRectF(opt.rect).adjusted(0.5, 0.5, -0.5, -0.5);
    painter->setPen(QColor::fromRgb(150, 150, 150));
    painter->drawLine(innerRect.bottomLeft(), innerRect.bottomRight());
    painter->restore();
}

}
