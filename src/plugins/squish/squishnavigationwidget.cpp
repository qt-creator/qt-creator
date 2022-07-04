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

#include "squishnavigationwidget.h"
#include "squishconstants.h"
#include "squishfilehandler.h"
#include "squishtesttreemodel.h"
#include "squishtesttreeview.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

namespace Squish {
namespace Internal {

const int defaultSectionSize = 17;

SquishNavigationWidget::SquishNavigationWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Squish"));
    m_view = new SquishTestTreeView(this);
    m_model = SquishTestTreeModel::instance();
    m_sortModel = new SquishTestTreeSortModel(m_model, m_model);
    m_sortModel->setDynamicSortFilter(true);
    m_view->setModel(m_sortModel);
    m_view->setSortingEnabled(true);
    m_view->setItemDelegate(new SquishTestTreeItemDelegate(this));
    QHeaderView *header = new QHeaderView(Qt::Horizontal, m_view);
    header->setModel(m_model);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(16);
    header->setDefaultSectionSize(16);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    m_view->setHeader(header);
    m_view->setHeaderHidden(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_view));
    setLayout(layout);

    connect(m_view, &QTreeView::expanded, this, &SquishNavigationWidget::onExpanded);
    connect(m_view, &QTreeView::collapsed, this, &SquishNavigationWidget::onCollapsed);
    connect(m_view, &QTreeView::activated, this, &SquishNavigationWidget::onItemActivated);
    connect(m_model,
            &QAbstractItemModel::rowsInserted,
            this,
            &SquishNavigationWidget::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &SquishNavigationWidget::onRowsRemoved);
    connect(m_view,
            &SquishTestTreeView::runTestCase,
            SquishFileHandler::instance(),
            &SquishFileHandler::runTestCase);
    connect(m_view,
            &SquishTestTreeView::runTestSuite,
            SquishFileHandler::instance(),
            &SquishFileHandler::runTestSuite);
    connect(m_view,
            &SquishTestTreeView::openObjectsMap,
            SquishFileHandler::instance(),
            &SquishFileHandler::openObjectsMap);
    connect(SquishFileHandler::instance(), &SquishFileHandler::suitesOpened, this, [this]() {
        const QModelIndex &suitesIndex = m_view->model()->index(1, 0);
        if (m_view->isExpanded(suitesIndex))
            onExpanded(suitesIndex);
    });
}

SquishNavigationWidget::~SquishNavigationWidget() {}

void SquishNavigationWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    // item specific menu entries
    const QModelIndexList list = m_view->selectionModel()->selectedIndexes();
    if (list.size() == SquishTestTreeModel::COLUMN_COUNT) {
        QRect rect(m_view->visualRect(list.first()));
        if (rect.contains(event->pos())) {
            const QModelIndex &idx = list.first();
            const int type = idx.data(TypeRole).toInt();
            switch (type) {
            case SquishTestTreeItem::SquishTestCase: {
                const QString caseName = idx.data(DisplayNameRole).toString();
                const QString suiteName = idx.parent().data(DisplayNameRole).toString();
                QAction *runThisTestCase = new QAction(tr("Run This Test Case"), &menu);
                menu.addAction(runThisTestCase);
                QAction *deleteTestCase = new QAction(tr("Delete Test Case"), &menu);
                menu.addAction(deleteTestCase);
                menu.addSeparator();

                connect(runThisTestCase, &QAction::triggered, [suiteName, caseName]() {
                    SquishFileHandler::instance()->runTestCase(suiteName, caseName);
                });
                break;
            }
            case SquishTestTreeItem::SquishSuite: {
                const QString suiteName = idx.data(DisplayNameRole).toString();
                QAction *runThisTestSuite = new QAction(tr("Run This Test Suite"), &menu);
                menu.addAction(runThisTestSuite);
                menu.addSeparator();
                QAction *addNewTestCase = new QAction(tr("Add New Test Case..."), &menu);
                menu.addAction(addNewTestCase);
                QAction *closeTestSuite = new QAction(tr("Close Test Suite"), &menu);
                menu.addAction(closeTestSuite);
                QAction *deleteTestSuite = new QAction(tr("Delete Test Suite"), &menu);
                menu.addAction(deleteTestSuite);
                menu.addSeparator();

                connect(runThisTestSuite, &QAction::triggered, [suiteName]() {
                    SquishFileHandler::instance()->runTestSuite(suiteName);
                });
                connect(closeTestSuite, &QAction::triggered, [suiteName]() {
                    SquishFileHandler::instance()->closeTestSuite(suiteName);
                });
                break;
            }
            case SquishTestTreeItem::SquishSharedFile: {
                QAction *deleteSharedFile = new QAction(tr("Delete Shared File"), &menu);
                menu.addAction(deleteSharedFile);
                break;
            }
            case SquishTestTreeItem::SquishSharedFolder: {
                QAction *addSharedFile = new QAction(tr("Add Shared File"), &menu);
                menu.addAction(addSharedFile);
                // only add the action 'Remove Shared Folder' for top-level shared folders, not
                // to their recursively added sub-folders
                if (idx.parent().data(TypeRole).toInt() == SquishTestTreeItem::Root) {
                    QAction *removeSharedFolder = new QAction(tr("Remove Shared Folder"), &menu);
                    menu.addAction(removeSharedFolder);
                    menu.addSeparator();
                    connect(removeSharedFolder, &QAction::triggered, this, [this, idx]() {
                        onRemoveSharedFolderTriggered(idx.row(), idx.parent());
                    });
                }
                break;
            }
            default:
                break;
            }
        }
    }
    const QModelIndex &foldersIndex = m_view->model()->index(0, 0);
    const QModelIndex &suitesIndex = m_view->model()->index(1, 0);

    // general squish related menu entries
    QAction *openSquishSuites = new QAction(tr("Open Squish Suites..."), &menu);
    menu.addAction(openSquishSuites);
    QAction *createNewTestSuite = new QAction(tr("Create New Test Suite..."), &menu);
    menu.addAction(createNewTestSuite);

    connect(openSquishSuites,
            &QAction::triggered,
            SquishFileHandler::instance(),
            &SquishFileHandler::openTestSuites);

    if (m_view->model()->rowCount(suitesIndex) > 0) {
        menu.addSeparator();
        QAction *closeAllSuites = new QAction(tr("Close All Test Suites"), &menu);
        menu.addAction(closeAllSuites);

        connect(closeAllSuites, &QAction::triggered, this, [this]() {
            if (QMessageBox::question(this,
                                      tr("Close All Test Suites"),
                                      tr("Close all test suites?"
                                         /*"\nThis will close all related files as well."*/))
                == QMessageBox::Yes)
                SquishFileHandler::instance()->closeAllTestSuites();
        });
    }

    menu.addSeparator();
    QAction *addSharedFolder = new QAction(tr("Add Shared Folder..."), &menu);
    menu.addAction(addSharedFolder);

    connect(addSharedFolder,
            &QAction::triggered,
            SquishFileHandler::instance(),
            &SquishFileHandler::addSharedFolder);

    if (m_view->model()->rowCount(foldersIndex) > 0) {
        menu.addSeparator();
        QAction *removeAllFolders = new QAction(tr("Remove All Shared Folders"), &menu);
        menu.addAction(removeAllFolders);

        connect(removeAllFolders,
                &QAction::triggered,
                this,
                &SquishNavigationWidget::onRemoveAllSharedFolderTriggered);
    }

    menu.exec(mapToGlobal(event->pos()));
}

QList<QToolButton *> SquishNavigationWidget::createToolButtons()
{
    QList<QToolButton *> toolButtons;
    return toolButtons;
}

void SquishNavigationWidget::onItemActivated(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;

    SquishTestTreeItem *item = static_cast<SquishTestTreeItem *>(m_sortModel->itemFromIndex(idx));
    if (item->type() == SquishTestTreeItem::SquishSharedFolder)
        return;

    if (!item->filePath().isEmpty())
        Core::EditorManager::openEditor(Utils::FilePath::fromString(item->filePath()));
}

void SquishNavigationWidget::onExpanded(const QModelIndex &idx)
{
    if (idx.data().toString().startsWith(tr("Test Suites")))
        m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void SquishNavigationWidget::onCollapsed(const QModelIndex &idx)
{
    if (idx.data().toString().startsWith(tr("Test Suites")))
        m_view->header()->setDefaultSectionSize(0);
}

void SquishNavigationWidget::onRowsInserted(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(tr("Test Suites")))
        if (m_view->isExpanded(parent) && m_model->rowCount(parent))
            m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void SquishNavigationWidget::onRowsRemoved(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(tr("Test Suites")))
        if (m_model->rowCount(parent) == 0)
            m_view->header()->setDefaultSectionSize(0);
}

void SquishNavigationWidget::onRemoveSharedFolderTriggered(int row, const QModelIndex &parent)
{
    const QString folder = m_model->index(row, 0, parent).data().toString();
    QTC_ASSERT(!folder.isEmpty(), return );

    if (QMessageBox::question(Core::ICore::dialogParent(),
                              tr("Remove Shared Folder"),
                              tr("Remove \"%1\" from the list of shared folders?")
                                  .arg(QDir::toNativeSeparators(folder)))
        != QMessageBox::Yes) {
        return;
    }

    const QModelIndex &realIdx = m_sortModel->mapToSource(m_model->index(row, 0, parent));
    if (SquishFileHandler::instance()->removeSharedFolder(folder))
        m_model->removeTreeItem(realIdx.row(), realIdx.parent());
}

void SquishNavigationWidget::onRemoveAllSharedFolderTriggered()
{
    if (QMessageBox::question(Core::ICore::dialogParent(),
                              tr("Remove All Shared Folders"),
                              tr("Remove all shared folders?"))
        != QMessageBox::Yes) {
        return;
    }

    SquishFileHandler::instance()->removeAllSharedFolders();
    m_model->removeAllSharedFolders();
}

SquishNavigationWidgetFactory::SquishNavigationWidgetFactory()
{
    setDisplayName(tr("Squish"));
    setId(Squish::Constants::SQUISH_ID);
    setPriority(777);
}

Core::NavigationView SquishNavigationWidgetFactory::createWidget()
{
    SquishNavigationWidget *squishNavigationWidget = new SquishNavigationWidget;
    Core::NavigationView view;
    view.widget = squishNavigationWidget;
    view.dockToolBarWidgets = squishNavigationWidget->createToolButtons();
    return view;
}

} // namespace Internal
} // namespace Squish
