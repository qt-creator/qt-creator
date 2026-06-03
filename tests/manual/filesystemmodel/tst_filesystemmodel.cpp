// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filesystemmodel.h>
#include <utils/filepath.h>

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QListView>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto model = new Utils::FileSystemModel;

    auto proxy = new Utils::FileSystemProxyModel;
    proxy->setSourceModel(model);
    proxy->setSortRole(Utils::FileSystemModel::FileSortRole);
    proxy->sort(0);

    // Tree view
    auto treeView = new QTreeView;
    treeView->setModel(proxy);
    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    treeView->setAnimated(true);
    treeView->setUniformRowHeights(true);

    // List view — icon mode with large icons
    auto listView = new QListView;
    listView->setModel(proxy);
    listView->setViewMode(QListView::IconMode);
    listView->setIconSize(QSize(64, 64));
    listView->setGridSize(QSize(100, 90));
    listView->setResizeMode(QListView::Adjust);
    listView->setUniformItemSizes(true);
    listView->setWordWrap(true);

    auto stack = new QStackedWidget;
    stack->addWidget(treeView); // index 0
    stack->addWidget(listView); // index 1

    // Toggle button
    auto upButton = new QToolButton;
    upButton->setText("Up");

    auto toggleButton = new QToolButton;
    toggleButton->setText("List");
    toggleButton->setCheckable(true);

    auto hiddenButton = new QToolButton;
    hiddenButton->setText("Show Hidden");
    hiddenButton->setCheckable(true);

    auto sortRoleCombo = new QComboBox;
    sortRoleCombo->addItem("Folders first", Utils::FileSystemModel::FileSortRole);
    sortRoleCombo->addItem("Name",          Utils::FileSystemModel::FileNameRole);
    sortRoleCombo->addItem("File path",     Utils::FileSystemModel::FilePathRole);
    sortRoleCombo->addItem("Default",       int(Qt::DisplayRole));

    auto pathEdit = new QLineEdit;
    pathEdit->setPlaceholderText("Enter path…");
    pathEdit->setText(QDir::homePath());

    auto statusBar = new QStatusBar;

    auto *window = new QWidget;
    window->setWindowTitle("FileSystemModel Test");
    window->resize(900, 600);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(upButton);
    topLayout->addWidget(new QLabel("Root:"));
    topLayout->addWidget(pathEdit);
    topLayout->addWidget(toggleButton);
    topLayout->addWidget(hiddenButton);
    topLayout->addWidget(new QLabel("Sort:"));
    topLayout->addWidget(sortRoleCombo);

    auto *mainLayout = new QVBoxLayout(window);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(stack);
    mainLayout->addWidget(statusBar);

    // Applies a new root path to the model and updates both views.
    auto applyPath = [&](const Utils::FilePath &path) {
        pathEdit->setText(path.toUserOutput());
        model->setRootPath(path);
        const QModelIndex sourceRoot = path.isEmpty() ? QModelIndex{} : model->index(path);
        const QModelIndex proxyRoot = proxy->mapFromSource(sourceRoot);
        treeView->setRootIndex(proxyRoot);
        listView->setRootIndex(proxyRoot);
    };

    QObject::connect(pathEdit, &QLineEdit::returnPressed, window, [&] {
        applyPath(Utils::FilePath::fromUserInput(pathEdit->text()));
    });

    QObject::connect(upButton, &QToolButton::clicked, window, [&] {
        const Utils::FilePath current = model->rootPath();
        if (current.isEmpty())
            return;
        const Utils::FilePath parent = current.parentDir();
        applyPath(parent == current ? Utils::FilePath{} : parent);
    });

    QObject::connect(toggleButton, &QToolButton::toggled, window, [&](bool listMode) {
        toggleButton->setText(listMode ? "Tree" : "List");
        stack->setCurrentIndex(listMode ? 1 : 0);
    });

    QObject::connect(hiddenButton, &QToolButton::toggled, window, [&](bool show) {
        proxy->setShowHiddenFiles(show);
    });

    QObject::connect(sortRoleCombo, &QComboBox::currentIndexChanged, window, [&](int index) {
        proxy->setSortRole(sortRoleCombo->itemData(index).toInt());
    });

    // Double-click in list view: navigate into directories.
    QObject::connect(listView, &QListView::doubleClicked, window, [&](const QModelIndex &index) {
        const QModelIndex source = proxy->mapToSource(index);
        const Utils::FilePath path = model->filePath(source);
        if (model->isDir(source))
            applyPath(path);
    });

    QObject::connect(model, &Utils::FileSystemModel::directoryLoaded,
                     window, [statusBar](const Utils::FilePath &path) {
        statusBar->showMessage("Loaded: " + path.toUserOutput(), 3000);
    });

    // Show selected path in status bar for whichever view is active.
    auto onCurrentChanged = [&](const QModelIndex &current) {
        const QModelIndex source = proxy->mapToSource(current);
        statusBar->showMessage(model->filePath(source).toUserOutput());
    };
    QObject::connect(
        treeView->selectionModel(), &QItemSelectionModel::currentChanged, window, onCurrentChanged);
    QObject::connect(
        listView->selectionModel(), &QItemSelectionModel::currentChanged, window, onCurrentChanged);

    applyPath(Utils::FilePath::fromUserInput(pathEdit->text()));

    window->show();
    return app.exec();
}
