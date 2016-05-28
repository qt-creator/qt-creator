/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewnavigationwidget.h"
#include "classviewmanager.h"
#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"
#include "classviewutils.h"
#include "classviewconstants.h"

#include <coreplugin/find/itemviewfind.h>

#include <cplusplus/Icons.h>

#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QVariant>
#include <QVBoxLayout>

enum { debug = false };

namespace ClassView {
namespace Internal {

///////////////////////////////// NavigationWidget //////////////////////////////////

/*!
    \class NavigationWidget

    The NavigationWidget class is a widget for the class view tree.
*/


/*!
    \fn void NavigationWidget::visibilityChanged(bool visibility)

    Emits a signal when the widget visibility is changed. \a visibility returns
    true if plugin becames visible, otherwise it returns false.
*/

/*!
    \fn void NavigationWidget::requestGotoLocation(const QString &name,
                                                   int line,
                                                   int column)

    Emits a signal that requests to open the file with \a name at \a line
    and \a column.

   \sa Manager::gotoLocation
*/

/*!
    \fn void NavigationWidget::requestGotoLocations(const QList<QVariant> &locations)

    Emits a signal to request to go to any of the Symbol \a locations in the
    list.

   \sa Manager::gotoLocations
*/

/*!
    \fn void NavigationWidget::requestTreeDataUpdate()

    Emits a signal that the widget wants to receive the latest tree info.

   \sa Manager::onRequestTreeDataUpdate
*/

NavigationWidget::NavigationWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(0);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    treeView = new ::Utils::NavigationTreeView(this);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setDragEnabled(true);
    treeView->setDragDropMode(QAbstractItemView::DragOnly);
    treeView->setDefaultDropAction(Qt::MoveAction);
    treeView->setExpandsOnDoubleClick(false);
    verticalLayout->addWidget(Core::ItemViewFind::createSearchableWrapper(
                                  treeView, Core::ItemViewFind::DarkColored,
                                  Core::ItemViewFind::FetchMoreWhileSearching));

    // tree model
    treeModel = new TreeItemModel(this);
    treeView->setModel(treeModel);

    // connect signal/slots
    // selected item
    connect(treeView, &QAbstractItemView::activated, this, &NavigationWidget::onItemActivated);

    // double-clicked item
    connect(treeView, &QAbstractItemView::doubleClicked, this, &NavigationWidget::onItemDoubleClicked);

    // connections to the manager
    Manager *manager = Manager::instance();

    connect(this, &NavigationWidget::visibilityChanged,
            manager, &Manager::onWidgetVisibilityIsChanged);

    connect(this, &NavigationWidget::requestGotoLocation,
            manager, &Manager::gotoLocation);

    connect(this, &NavigationWidget::requestGotoLocations,
            manager, &Manager::gotoLocations);

    connect(manager, &Manager::treeDataUpdate,
            this, &NavigationWidget::onDataUpdate);

    connect(this, &NavigationWidget::requestTreeDataUpdate,
            manager, &Manager::onRequestTreeDataUpdate);
}

NavigationWidget::~NavigationWidget()
{
}

void NavigationWidget::hideEvent(QHideEvent *event)
{
    emit visibilityChanged(false);
    QWidget::hideEvent(event);
}

void NavigationWidget::showEvent(QShowEvent *event)
{
    emit visibilityChanged(true);

    // request to update to the current state - to be sure
    emit requestTreeDataUpdate();

    QWidget::showEvent(event);
}

/*!
    Creates QToolbuttons for the Navigation Pane widget.

    Returns the list of created QToolButtons.

   \sa NavigationWidgetFactory::createWidget
*/

QList<QToolButton *> NavigationWidget::createToolButtons()
{
    QList<QToolButton *> list;

    // full projects mode
    if (!fullProjectsModeButton) {
        // create a button
        fullProjectsModeButton = new QToolButton();
        fullProjectsModeButton->setIcon(
                    CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType));
        fullProjectsModeButton->setCheckable(true);
        fullProjectsModeButton->setToolTip(tr("Show Subprojects"));

        // by default - not a flat mode
        setFlatMode(false);

        // connections
        connect(fullProjectsModeButton.data(), &QAbstractButton::toggled,
                this, &NavigationWidget::onFullProjectsModeToggled);
    }

    list << fullProjectsModeButton;

    return list;
}

/*!
    Returns flat mode state.
*/

bool NavigationWidget::flatMode() const
{
    QTC_ASSERT(fullProjectsModeButton, return false);

    // button is 'full projects mode' - so it has to be inverted
    return !fullProjectsModeButton->isChecked();
}

/*!
   Sets the flat mode state to \a flatMode.
*/

void NavigationWidget::setFlatMode(bool flatMode)
{
    QTC_ASSERT(fullProjectsModeButton, return);

    // button is 'full projects mode' - so it has to be inverted
    fullProjectsModeButton->setChecked(!flatMode);
}

/*!
    Full projects mode button has been toggled. \a state holds the full
    projects mode.
*/

void NavigationWidget::onFullProjectsModeToggled(bool state)
{
    // button is 'full projects mode' - so it has to be inverted
    Manager::instance()->setFlatMode(!state);
}

/*!
    Activates the item with the \a index in the tree view.
*/

void NavigationWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QList<QVariant> list = treeModel->data(index, Constants::SymbolLocationsRole).toList();

    emit requestGotoLocations(list);
}

/*!
    Expands/collapses the item given by \a index if it
    refers to a project file (.pro/.pri)
*/

void NavigationWidget::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const QVariant iconType = treeModel->data(index, Constants::IconTypeRole);
    if (!iconType.isValid())
        return;

    bool ok = false;
    const int type = iconType.toInt(&ok);
    if (ok && type == INT_MIN)
        treeView->setExpanded(index, !treeView->isExpanded(index));
}

/*!
    Receives new data for the tree. \a result is a pointer to the Class View
    model root item. The function does nothing if null is passed.
*/

void NavigationWidget::onDataUpdate(QSharedPointer<QStandardItem> result)
{
    if (result.isNull())
        return;

    QTime timer;
    if (debug)
        timer.start();
    // update is received. root item must be updated - and received information
    // might be just a root - if a lazy data population is enabled.
    // so expanded items must be parsed and 'fetched'

    fetchExpandedItems(result.data(), treeModel->invisibleRootItem());

    treeModel->moveRootToTarget(result.data());

    // expand top level projects
    QModelIndex sessionIndex;

    for (int i = 0; i < treeModel->rowCount(sessionIndex); ++i)
        treeView->expand(treeModel->index(i, 0, sessionIndex));

    if (debug)
        qDebug() << "Class View:" << QDateTime::currentDateTime().toString()
            << "TreeView is updated in" << timer.elapsed() << "msecs";
}

/*!
    Fetches data for expanded items to make sure that the content will exist.
    \a item and \a target do nothing if null is passed.
*/

void NavigationWidget::fetchExpandedItems(QStandardItem *item, const QStandardItem *target) const
{
    if (!item || !target)
        return;

    const QModelIndex &parent = treeModel->indexFromItem(target);
    if (treeView->isExpanded(parent) && Manager::instance()->canFetchMore(item, true))
        Manager::instance()->fetchMore(item, true);

    int itemIndex = 0;
    int targetIndex = 0;
    int itemRows = item->rowCount();
    int targetRows = target->rowCount();

    while (itemIndex < itemRows && targetIndex < targetRows) {
        QStandardItem *itemChild = item->child(itemIndex);
        const QStandardItem *targetChild = target->child(targetIndex);

        const SymbolInformation &itemInf = Utils::symbolInformationFromItem(itemChild);
        const SymbolInformation &targetInf = Utils::symbolInformationFromItem(targetChild);

        if (itemInf < targetInf) {
            ++itemIndex;
        } else if (itemInf == targetInf) {
            fetchExpandedItems(itemChild, targetChild);
            ++itemIndex;
            ++targetIndex;
        } else {
            ++targetIndex;
        }
    }
}

} // namespace Internal
} // namespace ClassView
