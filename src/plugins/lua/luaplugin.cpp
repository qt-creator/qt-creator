// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "luaengine.h"
#include "luapluginspec.h"
#include "luatr.h"

#include <coreplugin/icore.h>
#include <coreplugin/ioutputpane.h>
#include <coreplugin/jsexpander.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>
#include <QStyledItemDelegate>

using namespace Core;
using namespace Utils;
using namespace ExtensionSystem;

namespace Lua::Internal {

void setupActionModule();
void setupAsyncModule();
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
        label->setText(text);
        label->setFont(option.font);
        label->setTextInteractionFlags(
            Qt::TextInteractionFlag::TextSelectableByMouse
            | Qt::TextInteractionFlag::TextSelectableByKeyboard);
        label->setAutoFillBackground(true);
        label->setSelection(0, text.size());
        return label;
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
        f.open(QIODevice::ReadOnly);
        const auto ilua = QString::fromUtf8(f.readAll());
        m_luaState = runScript(ilua, "ilua.lua", [this](sol::state &lua) {
            lua["print"] = [this](sol::variadic_args va) {
                const QString msgs = variadicToStringList(va).join("\t").replace("\r\n", "\n");
                m_model.setStringList(m_model.stringList() << msgs);
                scrollToBottom();
            };

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

public:
    LuaPlugin() {}

    void initialize() final
    {
        setupLuaEngine(this);

        setupActionModule();
        setupAsyncModule();
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
        setupTextEditorModule();
        setupTranslateModule();
        setupUtilsModule();

        Core::JsExpander::registerGlobalObject("Lua", [] { return new LuaJsExtension(); });

        setupLuaExpander(globalMacroExpander());

        pluginSpecsFromArchiveFactories().push_back([](const FilePath &path) {
            QList<PluginSpec *> plugins;
            auto dirs = path.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
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
            FilePaths folders = path.dirEntries(FileFilter({}, QDir::Dirs | QDir::NoDotAndDotDot));

            for (const FilePath &folder : folders) {
                const FilePath script = folder / (folder.baseName() + ".lua");
                if (!script.exists())
                    continue;

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
};

} // namespace Lua::Internal

#include "luaplugin.moc"
