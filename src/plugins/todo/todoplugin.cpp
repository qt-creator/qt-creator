// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputpane.h"
#include "todoitemsprovider.h"
#include "todoprojectpanel.h"
#include "todotr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <extensionsystem/iplugin.h>

#ifdef WITH_TESTS
#include "todoitemsmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <utils/temporarydirectory.h>

#include <QIdentityProxyModel>
#include <QTemporaryFile>
#include <QTreeView>
#include <QtTest>
#endif

namespace Todo::Internal {

#ifdef WITH_TESTS
class TodoNavigationTest final : public QObject
{
    Q_OBJECT

private slots:
    void testNavigationUsesOriginalFilePath()
    {
        QTemporaryFile file(Utils::TemporaryDirectory::masterDirectoryFilePath()
                                .pathAppended("todo-XXXXXX.txt").toFSPathString());
        QVERIFY(file.open());
        QVERIFY(file.write("first\nsecond\nthird\nfourth\n") > 0);
        QVERIFY(file.flush());

        TodoItem first;
        first.file = Utils::FilePath::fromString(file.fileName());
        first.text = "TODO first";
        first.line = 2;
        QVERIFY2(first.file.exists(), qPrintable(first.file.toUserOutput()));
        TodoItem second = first;
        second.text = "TODO second";
        second.line = 4;
        QList<TodoItem> items{first, second};
        TodoItemsModel model;
        model.setTodoItemsList(&items);
        TodoOutputPane pane(&model, nullptr);
        auto view = qobject_cast<QTreeView *>(pane.outputWidget(nullptr));
        QVERIFY(view);
        view->sortByColumn(Constants::OUTPUT_COLUMN_TEXT, Qt::AscendingOrder);

        class RelativePathModel final : public QIdentityProxyModel
        {
        public:
            QVariant data(const QModelIndex &index, int role) const override
            {
                if (index.column() == Constants::OUTPUT_COLUMN_FILE && role == Qt::DisplayRole)
                    return "relative/todo.txt";
                return QIdentityProxyModel::data(index, role);
            }
        } relativePathModel;
        relativePathModel.setSourceModel(view->model());
        view->setModel(&relativePathModel);

        for (int column = 0; column < Constants::OUTPUT_COLUMN_COUNT; ++column) {
            const QModelIndex index = relativePathModel.index(0, column);
            QCOMPARE(index.data(Qt::UserRole).value<TodoItem>().file, first.file);
            view->setCurrentIndex(index);
            view->activated(index);
            Core::IEditor *editor = Core::EditorManager::currentEditor();
            QVERIFY(editor);
            QCOMPARE(editor->document()->filePath(), first.file);
            QCOMPARE(editor->currentLine(), first.line);
        }

        pane.goToNext();
        QCOMPARE(Core::EditorManager::currentEditor()->currentLine(), second.line);
        pane.goToPrev();
        QCOMPARE(Core::EditorManager::currentEditor()->currentLine(), first.line);
        QVERIFY(Core::EditorManager::closeEditors({Core::EditorManager::currentEditor()}, false));
    }
};
#endif

class TodoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Todo.json")

public:
    TodoPlugin()
    {
        qRegisterMetaType<TodoItem>("TodoItem");
    }

    void initialize() final
    {
#ifdef WITH_TESTS
        addTest<TodoNavigationTest>();
#endif
        Core::IOptionsPage::registerCategory(
            "To-Do", Tr::tr("To-Do"), ":/todoplugin/images/settingscategory_todo.png");

        todoSettings().load();

        setupTodoItemsProvider(this);
        setupTodoOutputPane(this);

        setupTodoSettingsPage();

        setupTodoProjectPanel();
    }
};

} // Todo::Internal

#include "todoplugin.moc"
