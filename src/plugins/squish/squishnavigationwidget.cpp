// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishnavigationwidget.h"

#include "squishconstants.h"
#include "squishfilehandler.h"
#include "squishmessages.h"
#include "squishsettings.h"
#include "squishtesttreemodel.h"
#include "squishtesttreeview.h"
#include "squishtr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>

#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

using namespace Utils;

namespace Squish::Internal {

const int defaultSectionSize = 17;

class SquishNavigationWidget : public QWidget
{
public:
    explicit SquishNavigationWidget(QWidget *parent = nullptr);
    ~SquishNavigationWidget() override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    static QList<QToolButton *> createToolButtons();

private:
    void onItemActivated(const QModelIndex &idx);
    void onExpanded(const QModelIndex &idx);
    void onCollapsed(const QModelIndex &idx);
    void onRowsInserted(const QModelIndex &parent, int, int);
    void onRowsRemoved(const QModelIndex &parent, int, int);
    void onRemoveSharedFolderTriggered(int row, const QModelIndex &parent);
    void onRemoveAllSharedFolderTriggered();
    void onRecordTestCase(const QString &suiteName, const QString &testCase);
    void onNewTestCaseTriggered(const QModelIndex &index);

    SquishTestTreeView *m_view;
    SquishTestTreeModel *m_model; // not owned
    SquishTestTreeSortModel *m_sortModel;
};


SquishNavigationWidget::SquishNavigationWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(Tr::tr("Squish"));
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
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_view));
    setLayout(layout);

    connect(m_view, &QTreeView::expanded, this, &SquishNavigationWidget::onExpanded);
    connect(m_view, &QTreeView::collapsed, this, &SquishNavigationWidget::onCollapsed);
    connect(m_view, &QTreeView::activated, this, &SquishNavigationWidget::onItemActivated);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &SquishNavigationWidget::onRowsInserted);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, &SquishNavigationWidget::onRowsRemoved);
    connect(m_view, &SquishTestTreeView::runTestCase,
            SquishFileHandler::instance(), &SquishFileHandler::runTestCase);
    connect(m_view, &SquishTestTreeView::recordTestCase,
            this, &SquishNavigationWidget::onRecordTestCase);
    connect(m_view, &SquishTestTreeView::runTestSuite,
            SquishFileHandler::instance(), &SquishFileHandler::runTestSuite);
    connect(m_view, &SquishTestTreeView::openObjectsMap,
            SquishFileHandler::instance(), &SquishFileHandler::openObjectsMap);
    connect(SquishFileHandler::instance(), &SquishFileHandler::suitesOpened, this, [this] {
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
                QAction *runThisTestCase = new QAction(Tr::tr("Run This Test Case"), &menu);
                menu.addAction(runThisTestCase);
                QAction *deleteTestCase = new QAction(Tr::tr("Delete Test Case"), &menu);
                menu.addAction(deleteTestCase);
                menu.addSeparator();

                connect(runThisTestCase, &QAction::triggered, [suiteName, caseName] {
                    SquishFileHandler::instance()->runTestCase(suiteName, caseName);
                });
                connect(deleteTestCase, &QAction::triggered, [suiteName, caseName] {
                    SquishFileHandler::instance()->deleteTestCase(suiteName, caseName);
                });
                break;
            }
            case SquishTestTreeItem::SquishSuite: {
                const QString suiteName = idx.data(DisplayNameRole).toString();
                QAction *runThisTestSuite = new QAction(Tr::tr("Run This Test Suite"), &menu);
                menu.addAction(runThisTestSuite);
                menu.addSeparator();
                QAction *addNewTestCase = new QAction(Tr::tr("Add New Test Case..."), &menu);
                menu.addAction(addNewTestCase);
                QAction *closeTestSuite = new QAction(Tr::tr("Close Test Suite"), &menu);
                menu.addAction(closeTestSuite);
                menu.addSeparator();

                connect(runThisTestSuite, &QAction::triggered, [suiteName] {
                    SquishFileHandler::instance()->runTestSuite(suiteName);
                });
                connect(addNewTestCase, &QAction::triggered, this, [this, idx] {
                    onNewTestCaseTriggered(idx);
                });

                connect(closeTestSuite, &QAction::triggered, [suiteName] {
                    SquishFileHandler::instance()->closeTestSuite(suiteName);
                });
                break;
            }
            case SquishTestTreeItem::SquishSharedFile: {
                QAction *deleteSharedFile = new QAction(Tr::tr("Delete Shared File"), &menu);
                menu.addAction(deleteSharedFile);
                break;
            }
            case SquishTestTreeItem::SquishSharedFolder: {
                QAction *addSharedFile = new QAction(Tr::tr("Add Shared File"), &menu);
                menu.addAction(addSharedFile);
                // only add the action 'Remove Shared Folder' for top-level shared folders, not
                // to their recursively added sub-folders
                if (idx.parent().data(TypeRole).toInt() == SquishTestTreeItem::Root) {
                    QAction *removeSharedFolder = new QAction(Tr::tr("Remove Shared Folder"), &menu);
                    menu.addAction(removeSharedFolder);
                    menu.addSeparator();
                    connect(removeSharedFolder, &QAction::triggered, this, [this, idx] {
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
    QAction *openSquishSuites = new QAction(Tr::tr("Open Squish Suites..."), &menu);
    menu.addAction(openSquishSuites);
    QAction *createNewTestSuite = new QAction(Tr::tr("Create New Test Suite..."), &menu);
    menu.addAction(createNewTestSuite);

    connect(createNewTestSuite, &QAction::triggered, this, [] {
        auto command = Core::ActionManager::command(Utils::Id("Wizard.Impl.S.SquishTestSuite"));
        if (command && command->action())
            command->action()->trigger();
        else
            qWarning("Failed to get wizard command. UI changed?");
    });
    connect(openSquishSuites,
            &QAction::triggered,
            SquishFileHandler::instance(),
            &SquishFileHandler::openTestSuites);

    if (m_view->model()->rowCount(suitesIndex) > 0) {
        menu.addSeparator();
        QAction *closeAllSuites = new QAction(Tr::tr("Close All Test Suites"), &menu);
        menu.addAction(closeAllSuites);

        connect(closeAllSuites, &QAction::triggered, this, [] {
            if (SquishMessages::simpleQuestion(Tr::tr("Close All Test Suites"),
                                               Tr::tr("Close all test suites?"
                                                      /*"\nThis will close all related files as well."*/))
                == QMessageBox::Yes)
                SquishFileHandler::instance()->closeAllTestSuites();
        });
    }

    menu.addSeparator();
    QAction *addSharedFolder = new QAction(Tr::tr("Add Shared Folder..."), &menu);
    menu.addAction(addSharedFolder);

    connect(addSharedFolder,
            &QAction::triggered,
            SquishFileHandler::instance(),
            &SquishFileHandler::addSharedFolder);

    if (m_view->model()->rowCount(foldersIndex) > 0) {
        menu.addSeparator();
        QAction *removeAllFolders = new QAction(Tr::tr("Remove All Shared Folders"), &menu);
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
    switch (item->type()) {
    case SquishTestTreeItem::SquishSharedDataFolder:
    case SquishTestTreeItem::SquishSharedFolder:
    case SquishTestTreeItem::SquishSharedRoot:
        return;
    default:
        break;
    }

    if (item->filePath().exists())
        Core::EditorManager::openEditor(item->filePath());
}

void SquishNavigationWidget::onExpanded(const QModelIndex &idx)
{
    if (idx.data().toString().startsWith(Tr::tr("Test Suites")))
        m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void SquishNavigationWidget::onCollapsed(const QModelIndex &idx)
{
    if (idx.data().toString().startsWith(Tr::tr("Test Suites")))
        m_view->header()->setDefaultSectionSize(0);
}

void SquishNavigationWidget::onRowsInserted(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(Tr::tr("Test Suites")))
        if (m_view->isExpanded(parent) && m_model->rowCount(parent))
            m_view->header()->setDefaultSectionSize(defaultSectionSize);
}

void SquishNavigationWidget::onRowsRemoved(const QModelIndex &parent, int, int)
{
    if (parent.isValid() && parent.data().toString().startsWith(Tr::tr("Test Suites")))
        if (m_model->rowCount(parent) == 0)
            m_view->header()->setDefaultSectionSize(0);
}

void SquishNavigationWidget::onRemoveSharedFolderTriggered(int row, const QModelIndex &parent)
{
    const auto folder = Utils::FilePath::fromVariant(m_sortModel->index(row, 0, parent).data(LinkRole));
    QTC_ASSERT(!folder.isEmpty(), return );

    const QString detail = Tr::tr("Remove \"%1\" from the list of shared folders?")
            .arg(folder.toUserOutput());
    if (SquishMessages::simpleQuestion(Tr::tr("Remove Shared Folder"), detail) != QMessageBox::Yes)
        return;

    const QModelIndex &realIdx = m_sortModel->mapToSource(m_sortModel->index(row, 0, parent));
    if (SquishFileHandler::instance()->removeSharedFolder(folder))
        m_model->removeTreeItem(realIdx.row(), realIdx.parent());
}

void SquishNavigationWidget::onRemoveAllSharedFolderTriggered()
{
    if (SquishMessages::simpleQuestion(Tr::tr("Remove All Shared Folders"),
                                       Tr::tr("Remove all shared folders?")) != QMessageBox::Yes) {
        return;
    }

    SquishFileHandler::instance()->removeAllSharedFolders();
    m_model->removeAllSharedFolders();
}

void SquishNavigationWidget::onRecordTestCase(const QString &suiteName, const QString &testCase)
{
    QMessageBox::StandardButton pressed = CheckableMessageBox::question(
        Core::ICore::dialogParent(),
        Tr::tr("Record Test Case"),
        Tr::tr("Do you want to record over the test case \"%1\"? The existing content will "
               "be overwritten by the recorded script.")
            .arg(testCase),
        Key("RecordWithoutApproval"));
    if (pressed != QMessageBox::Yes)
        return;

    SquishFileHandler::instance()->recordTestCase(suiteName, testCase);
}

void SquishNavigationWidget::onNewTestCaseTriggered(const QModelIndex &index)
{
    if (!settings().squishPath().pathAppended("scriptmodules").exists()) {
        SquishMessages::criticalMessage(Tr::tr("Set up a valid Squish path to be able to create "
                                               "a new test case.\n(Edit > Preferences > Squish)"));
        return;
    }

    SquishTestTreeItem *suiteItem = m_model->itemForIndex(m_sortModel->mapToSource(index));
    QTC_ASSERT(suiteItem, return);

    const QString name = suiteItem->generateTestCaseName();
    SquishTestTreeItem *item = new SquishTestTreeItem(name, SquishTestTreeItem::SquishTestCase);
    item->setParentName(suiteItem->displayName());

    m_model->addTreeItem(item);
    m_view->expand(index);
    QModelIndex added = m_model->indexForItem(item);
    QTC_ASSERT(added.isValid(), return);
    m_view->edit(m_sortModel->mapFromSource(added));
}

SquishNavigationWidgetFactory::SquishNavigationWidgetFactory()
{
    setDisplayName(Tr::tr("Squish"));
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

} // Squish::Internal
