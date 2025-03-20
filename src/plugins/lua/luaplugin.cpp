// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luapluginspec.h"
#include "luatr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QMenu>
#include <QToolBar>
#include <QStringListModel>
#include <QStyledItemDelegate>

using namespace Core;
using namespace Utils;
using namespace ExtensionSystem;

namespace Lua::Internal {

const char M_SCRIPT[] = "Lua.Script";
const char G_SCRIPTS[] = "Lua.Scripts";
const char ACTION_SCRIPTS_BASE[] = "Lua.Scripts.";
const char ACTION_NEW_SCRIPT[] = "Lua.NewScript";

void setupActionModule();
void setupCoreModule();
void setupFetchModule();
void setupGuiModule();
void setupHookModule();
void setupInstallModule();
void setupJsonModule();
void setupLocalSocketModule();
void setupMacroModule();
void setupMessageManagerModule();
void setupProcessModule();
void setupProjectModule();
void setupQtModule();
void setupSettingsModule();
void setupTaskHubModule();
void setupTextEditorModule();
void setupTranslateModule();
void setupUtilsModule();

void setupLuaExpander(MacroExpander *expander);

class LuaJsExtension : public QObject
{
    Q_OBJECT

public:
    explicit LuaJsExtension(QObject *parent = nullptr)
        : QObject(parent)
    {}

    Q_INVOKABLE QString metaFolder() const
    {
        return Core::ICore::resourcePath("lua/meta").toFSPathString();
    }
};

class ItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(
        QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        auto label = new QLabel(parent);
        const QString text = index.data().toString();
        label->setText(text.startsWith("__ERROR__") ? text.mid(9) : text);
        label->setFont(option.font);
        label->setTextInteractionFlags(
            Qt::TextInteractionFlag::TextSelectableByMouse
            | Qt::TextInteractionFlag::TextSelectableByKeyboard);
        label->setAutoFillBackground(true);
        label->setSelection(0, text.size());
        return label;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        bool isError = opt.text.startsWith("__ERROR__");

        if (isError)
            opt.text = opt.text.mid(9);

        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, opt.palette.highlight());
            painter->setPen(opt.palette.highlightedText().color());
        } else if (isError) {
            painter->setPen(creatorColor(Theme::Token_Notification_Danger_Default));
        } else {
            painter->setPen(opt.palette.text().color());
        }

        painter->drawText(opt.rect, opt.displayAlignment, opt.text);
    }
};

class LuaReplView : public QListView
{
    Q_OBJECT

    std::unique_ptr<LuaState> m_luaState;
    sol::function m_readCallback;

    QStringListModel m_model;

public:
    LuaReplView(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setModel(&m_model);
        setItemDelegate(new ItemDelegate(this));
    }

    void showEvent(QShowEvent *) override
    {
        if (m_luaState) {
            return;
        }
        resetTerminal();
    }

    void handleRequestResult(const QString &result)
    {
        auto cb = m_readCallback;
        m_readCallback = {};
        cb(result);
    }

    void resetTerminal()
    {
        m_model.setStringList({});
        m_readCallback = {};

        QFile f(":/lua/scripts/ilua.lua");
        QTC_CHECK(f.open(QIODevice::ReadOnly));
        const auto ilua = QString::fromUtf8(f.readAll());
        m_luaState = runScript(ilua, "ilua.lua", [this](sol::state &lua) {
            lua["print"] = [this](sol::variadic_args va) {
                const QString msgs = variadicToStringList(va).join("\t").replace("\r\n", "\n");
                m_model.setStringList(m_model.stringList() << msgs);
                scrollToBottom();
            };
            lua["LuaCopyright"] = LUA_COPYRIGHT;

            sol::table async = lua.script("return require('async')", "_ilua_").get<sol::table>();
            sol::function wrap = async["wrap"];

            lua["readline_cb"] = [this](const QString &prompt, sol::function callback) {
                scrollToBottom();
                emit inputRequested(prompt);
                m_readCallback = callback;
            };

            lua["readline"] = wrap(lua["readline_cb"]);
        });

        QListView::reset();
    }

signals:
    void inputRequested(const QString &prompt);
};

class LineEdit : public FancyLineEdit
{
public:
    using FancyLineEdit::FancyLineEdit;
};

class LuaPane : public Core::IOutputPane
{
    Q_OBJECT

protected:
    QWidget *m_ui{nullptr};
    LuaReplView *m_terminal{nullptr};

public:
    LuaPane(QObject *parent = nullptr)
        : Core::IOutputPane(parent)
    {
        setId("LuaPane");
        setDisplayName(Tr::tr("Lua"));
        setPriorityInStatusBar(-20);
    }

    QWidget *outputWidget(QWidget *parent) override
    {
        using namespace Layouting;

        if (!m_ui && parent) {
            m_terminal = new LuaReplView;
            LineEdit *inputEdit = new LineEdit;
            QLabel *prompt = new QLabel;

            // clang-format off
            m_ui = Column {
                noMargin,
                spacing(0),
                m_terminal,
                Row { prompt, inputEdit },
            }.emerge();
            // clang-format on

            inputEdit->setReadOnly(true);
            inputEdit->setHistoryCompleter(Utils::Key("LuaREPL.InputHistory"), false, 200);

            connect(inputEdit, &QLineEdit::returnPressed, this, [this, inputEdit] {
                inputEdit->setReadOnly(true);
                m_terminal->handleRequestResult(inputEdit->text());
                inputEdit->onEditingFinished();
                inputEdit->clear();
            });
            connect(
                m_terminal,
                &LuaReplView::inputRequested,
                this,
                [prompt, inputEdit](const QString &p) {
                    prompt->setText(p);
                    inputEdit->setReadOnly(false);
                });
        }

        return m_ui;
    }

    void visibilityChanged(bool) override {};

    void clearContents() override
    {
        if (m_terminal)
            m_terminal->resetTerminal();
    }
    void setFocus() override { outputWidget(nullptr)->setFocus(); }
    bool hasFocus() const override { return true; }
    bool canFocus() const override { return true; }

    bool canNavigate() const override { return false; }
    bool canNext() const override { return false; }
    bool canPrevious() const override { return false; }
    void goToNext() override {}
    void goToPrev() override {}

    QList<QWidget *> toolBarWidgets() const override { return {}; }
};

class LuaPlugin : public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Lua.json")

private:
    LuaPane *m_pane = nullptr;
    std::unique_ptr<FilePathWatcher> m_userScriptsWatcher;

public:
    LuaPlugin() {}

    void initialize() final
    {
        IOptionsPage::registerCategory(
            "ZY.Lua", Tr::tr("Lua"), ":/lua/images/settingscategory_lua.png");

        setupLuaEngine(this);

        registerProvider("async", ":/lua/scripts/async.lua");
        registerProvider("inspect", ":/lua/scripts/inspect.lua");

        setupActionModule();
        setupCoreModule();
        setupFetchModule();
        setupGuiModule();
        setupHookModule();
        setupInstallModule();
        setupJsonModule();
        setupLocalSocketModule();
        setupMacroModule();
        setupMessageManagerModule();
        setupProcessModule();
        setupProjectModule();
        setupQtModule();
        setupSettingsModule();
        setupTaskHubModule();
        setupTextEditorModule();
        setupTranslateModule();
        setupUtilsModule();

        Core::JsExpander::registerGlobalObject("Lua", [] { return new LuaJsExtension(); });

        setupLuaExpander(globalMacroExpander());

        pluginSpecsFromArchiveFactories().push_back([](const FilePath &path) -> QList<PluginSpec *> {
            if (path.isFile()) {
                if (path.suffix() == "lua") {
                    Utils::expected_str<PluginSpec *> spec = loadPlugin(path);
                    QTC_CHECK_EXPECTED(spec);
                    if (spec)
                        return {*spec};
                }
                return {};
            }

            QList<PluginSpec *> plugins;
            const FilePaths dirs = path.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const auto &dir : dirs) {
                const auto specFilePath = dir / (dir.fileName() + ".lua");
                if (specFilePath.exists()) {
                    Utils::expected_str<PluginSpec *> spec = loadPlugin(specFilePath);
                    QTC_CHECK_EXPECTED(spec);
                    if (spec)
                        plugins.push_back(*spec);
                }
            }
            return plugins;
        });

        m_pane = new LuaPane(this);

        //register actions
        ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);

        ActionContainer *scriptContainer = ActionManager::createMenu(M_SCRIPT);

        Command *newScriptCommand = ActionBuilder(this, ACTION_NEW_SCRIPT)
                                        .setScriptable(true)
                                        .setText(Tr::tr("New Script..."))
                                        .addToContainer(M_SCRIPT)
                                        .addOnTriggered([]() {
                                            auto command = Core::ActionManager::command(Utils::Id("Wizard.Impl.Q.QCreatorScript"));
                                            if (command && command->action())
                                                command->action()->trigger();
                                            else
                                                qWarning("Failed to get wizard command. UI changed?");
                                        })
                                        .command();

        scriptContainer->addAction(newScriptCommand);
        scriptContainer->addSeparator();
        scriptContainer->appendGroup(G_SCRIPTS);

        scriptContainer->menu()->setTitle(Tr::tr("Scripting"));
        toolsContainer->addMenu(scriptContainer);

        const Utils::FilePath userScriptsPath = Core::ICore::userResourcePath("scripts");
        userScriptsPath.ensureWritableDir();
        if (auto watch = userScriptsPath.watch()) {
            m_userScriptsWatcher.swap(*watch);
            connect(
                m_userScriptsWatcher.get(),
                &FilePathWatcher::pathChanged,
                this,
                &LuaPlugin::scanForScripts);
        }

        scanForScripts();

        connect(
            EditorManager::instance(),
            &EditorManager::editorOpened,
            this,
            &LuaPlugin::onEditorOpened);

        ActionBuilder(this, Id(ACTION_SCRIPTS_BASE).withSuffix("current"))
            .setText(Tr::tr("Run Current Script"))
            .addOnTriggered([]() {
                if (auto textEditor = TextEditor::BaseTextEditor::currentTextEditor()) {
                    const FilePath path = textEditor->document()->filePath();
                    if (path.isChildOf(Core::ICore::userResourcePath("scripts")))
                        runScript(path);
                }
            });
    }

    bool delayedInitialize() final
    {
        scanForPlugins(PluginManager::pluginPaths());
        return true;
    }

    void scanForPlugins(const FilePaths &pluginPaths)
    {
        QSet<PluginSpec *> plugins;
        for (const FilePath &path : pluginPaths) {
            const FilePaths folders =
                path.dirEntries(FileFilter({}, QDir::Dirs | QDir::NoDotAndDotDot));

            for (const FilePath &folder : folders) {
                FilePath script = folder / (folder.baseName() + ".lua");
                if (!script.exists()) {
                    const FilePaths contents =
                        folder.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
                    if (contents.empty())
                        continue;

                    for (const FilePath &subfolder : contents) {
                        script = subfolder / (subfolder.baseName() + ".lua");
                        if (!script.exists()) {
                            script.clear();
                            continue;
                        }
                        break;
                    }
                }

                if (script.isEmpty()) {
                    continue;
                }

                const expected_str<LuaPluginSpec *> result = loadPlugin(script);

                if (!result) {
                    qWarning() << "Failed to load plugin" << script << ":" << result.error();
                    MessageManager::writeFlashing(Tr::tr("Failed to load plugin %1: %2")
                                                      .arg(script.toUserOutput())
                                                      .arg(result.error()));
                    continue;
                }

                plugins.insert(*result);
            }
        }

        PluginManager::addPlugins({plugins.begin(), plugins.end()});
        PluginManager::loadPluginsAtRuntime(plugins);
    }

    void scanForScripts()
    {
        const FilePath userScriptsPath = Core::ICore::userResourcePath("scripts");
        if (userScriptsPath.exists())
            scanForScriptsIn(userScriptsPath);

        const FilePath scriptsPath = Core::ICore::resourcePath("lua/scripts");
        if (scriptsPath.exists())
            scanForScriptsIn(scriptsPath);
    }

    void scanForScriptsIn(const FilePath &scriptsPath)
    {
        ActionContainer *scriptContainer = ActionManager::actionContainer(M_SCRIPT);

        const FilePaths scripts = scriptsPath.dirEntries(FileFilter({"*.lua"}, QDir::Files));
        for (const FilePath &script : scripts) {
            const Id base = Id(ACTION_SCRIPTS_BASE).withSuffix(script.baseName());
            const Id menuId = base.withSuffix(".Menu");
            if (!ActionManager::actionContainer(menuId)) {
                ActionContainer *container = ActionManager::createMenu(menuId);
                scriptContainer->addMenu(container);
                auto menu = container->menu();
                menu->setTitle(script.baseName());
                ActionBuilder(this, base)
                    .setText(script.baseName())
                    .setToolTip(Tr::tr("Run script \"%1\"").arg(script.toUserOutput()))
                    .addOnTriggered([script]() { runScript(script); });
                connect(menu->addAction(Tr::tr("Run")), &QAction::triggered, this, [script]() {
                    runScript(script);
                });
                connect(menu->addAction(Tr::tr("Edit")), &QAction::triggered, this, [script]() {
                    Core::EditorManager::openEditor(script);
                });
            }
        }
    }

    void onEditorOpened(Core::IEditor *editor)
    {
        const FilePath path = editor->document()->filePath();
        if (path.isChildOf(Core::ICore::userResourcePath("scripts"))) {
            auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            TextEditor::TextEditorWidget *editorWidget = textEditor->editorWidget();
            editorWidget->toolBar()
                ->addAction(Utils::Icons::RUN_SMALL_TOOLBAR.icon(), Tr::tr("Run"), [path]() {
                    runScript(path);
                });
        }
    }

    static void runScript(const FilePath &script)
    {
        expected_str<QByteArray> content = script.fileContents();
        if (content) {
            Lua::runScript(QString::fromUtf8(*content), script.fileName());
        } else {
            MessageManager::writeFlashing(Tr::tr("Failed to read script \"%1\": %2")
                                              .arg(script.toUserOutput())
                                              .arg(content.error()));
        }
    }
};

} // namespace Lua::Internal

#include "luaplugin.moc"
