/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
    Q_OBJECT
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
