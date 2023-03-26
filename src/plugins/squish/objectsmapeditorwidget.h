// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QItemSelection;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QStackedLayout;
class QTreeView;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
}

namespace Squish {
namespace Internal {

class ObjectsMapDocument;
class ObjectsMapSortFilterModel;
class ObjectsMapTreeItem;
class PropertiesSortModel;
class PropertyTreeItem;

class ObjectsMapEditorWidget : public QWidget
{
public:
    explicit ObjectsMapEditorWidget(ObjectsMapDocument *document, QWidget *parent = nullptr);

private:
    void initUi();
    void initializeConnections();
    void initializeContextMenus();
    void setPropertiesDisplayValid(bool valid);
    void onSelectionRequested(const QModelIndex &idx);
    void onObjectSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onPropertySelectionChanged(const QItemSelection &selected,
                                    const QItemSelection &deselected);
    void onPropertiesContentModified(const QString &text);
    void onJumpToSymbolicNameClicked();
    void onNewPropertyTriggered();
    void onRemovePropertyTriggered();
    void onNewSymbolicNameTriggered();
    void onRemoveSymbolicNameTriggered();
    void onCopySymbolTriggered();
    void onPasteSymbolicNameTriggered();
    void onCopyRealNameTriggered();
    void onCutSymbolicNameTriggered();
    void onCopyPropertyTriggered();
    void onCutPropertyTriggered();
    void onPastePropertyTriggered();
    QString ambiguousNameDialog(const QString &original,
                                const QStringList &usedNames,
                                bool isProperty);
    ObjectsMapTreeItem *selectedObjectItem() const;
    PropertyTreeItem *selectedPropertyItem() const;

    ObjectsMapDocument *m_document;
    ObjectsMapSortFilterModel *m_objMapFilterModel;
    PropertiesSortModel *m_propertiesSortModel;
    QMenu *m_symbolicNamesCtxtMenu;
    QMenu *m_propertiesCtxtMenu;

    Utils::FancyLineEdit *m_filterLineEdit;
    QTreeView *m_symbolicNamesTreeView;
    QTreeView *m_propertiesTree;
    QPushButton *m_newSymbolicName;
    QPushButton *m_removeSymbolicName;
    QPushButton *m_newProperty;
    QPushButton *m_removeProperty;
    QPushButton *m_jumpToSymbolicName;
    QLineEdit *m_propertiesLineEdit;
    QLabel *m_propertiesLabel;
    QStackedLayout *m_stackedLayout;
};

} // namespace Internal
} // namespace Squish
