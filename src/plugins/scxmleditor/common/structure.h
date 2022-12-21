// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmltypes.h"

#include <QFrame>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolbar)
QT_FORWARD_DECLARE_CLASS(QToolButton)

namespace ScxmlEditor {

namespace PluginInterface {
class ScxmlDocument;
class GraphicsScene;
}

namespace Common {

class StructureModel;
class TreeView;

class TreeItemDelegate : public QStyledItemDelegate
{
public:
    TreeItemDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class StructureSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit StructureSortFilterProxyModel(QObject *parent = nullptr);
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void setVisibleTags(const QVector<PluginInterface::TagType> &visibleTags);

private:
    QPointer<StructureModel> m_sourceModel;
    QVector<PluginInterface::TagType> m_visibleTags;
};

/**
 * @brief The Structure class provides the tree-view of the whole SCXML-document.
 */
class Structure : public QFrame
{
    Q_OBJECT

public:
    explicit Structure(QWidget *parent = nullptr);

    void setDocument(PluginInterface::ScxmlDocument *document);
    void setGraphicsScene(PluginInterface::GraphicsScene *scene);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void createUi();
    void rowActivated(const QModelIndex &index);
    void rowEntered(const QModelIndex &index);
    void childAdded(const QModelIndex &index);
    void currentTagChanged(const QModelIndex &sourceIndex);
    void showMenu(const QModelIndex &index, const QPoint &globalPos);
    void updateCheckBoxes();
    void addCheckbox(const QString &name, PluginInterface::TagType type);

    StructureSortFilterProxyModel *m_proxyModel;
    StructureModel *m_model;
    PluginInterface::ScxmlDocument *m_currentDocument = nullptr;
    PluginInterface::GraphicsScene *m_scene = nullptr;
    QVector<QCheckBox*> m_checkboxes;
    TreeItemDelegate *m_customDelegate = nullptr;

    QWidget *m_paneInnerFrame = nullptr;
    TreeView *m_structureView = nullptr;
    QToolButton *m_checkboxButton = nullptr;
    QLabel *m_visibleTagsTitle = nullptr;
    QWidget *m_tagVisibilityFrame = nullptr;
    QWidget *m_checkboxFrame = nullptr;
    QLayout *m_checkboxLayout = nullptr;
};

} // namespace Common
} // namespace ScxmlEditor
