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
#include <utils/qtcprocess.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>

using namespace Core;
using namespace Utils;
using namespace ExtensionSystem;

namespace Lua::Internal {

void addAsyncModule();
void addFetchModule();
void addActionModule();
void addUtilsModule();
void addMessageManagerModule();
void addProcessModule();
void addSettingsModule();
void addGuiModule();
void addQtModule();
void addCoreModule();
void addHookModule();
void addInstallModule();

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

class LuaTerminal : public QListView
{
    Q_OBJECT

    std::unique_ptr<LuaState> m_luaState;
    sol::function m_readCallback;

    QStringListModel m_model;

public:
    LuaTerminal(QWidget *parent = nullptr)
        : QListView(parent)
    {
        setModel(&m_model);
    }

    void showEvent(QShowEvent *) override
    {
        if (m_luaState) {
            return;
        }
        resetTerminal();
    }

    void keyPressEvent(QKeyEvent *e) override { emit keyPressed(e); }

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
        m_luaState = LuaEngine::instance().runScript(ilua, "ilua.lua", [this](sol::state &lua) {
            lua["print"] = [this](sol::variadic_args va) {
                const QStringList msgs = LuaEngine::variadicToStringList(va)
                                             .join("\t")
                                             .replace("\r\n", "\n")
                                             .split('\n');
                m_model.setStringList(m_model.stringList() + msgs);
                scrollToBottom();
            };

            sol::table async = lua.script("return require('async')", "_ilua_").get<sol::table>();
            sol::function wrap = async["wrap"];

            lua["readline_cb"] = [this](const QString &prompt, sol::function callback) {
                m_model.setStringList(m_model.stringList() << prompt);
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
    void keyPressed(QKeyEvent *e);
};

class LineEdit : public FancyLineEdit
{
public:
    using FancyLineEdit::FancyLineEdit;
    void keyPressEvent(QKeyEvent *e) override { FancyLineEdit::keyPressEvent(e); }
};

class LuaPane : public Core::IOutputPane
{
    Q_OBJECT

protected:
    QWidget *m_ui{nullptr};
    LuaTerminal *m_terminal{nullptr};

public:
    LuaPane(QObject *parent = nullptr)
        : Core::IOutputPane(parent)
    {
        setId("LuaPane");
        setDisplayName(Tr::tr("Lua"));
        setPriorityInStatusBar(20);
    }

    QWidget *outputWidget(QWidget *parent) override
    {
        using namespace Layouting;

        if (!m_ui && parent) {
            m_terminal = new LuaTerminal;
            LineEdit *inputEdit = new LineEdit;
            QLabel *prompt = new QLabel;

            // clang-format off
            m_ui = Column {
                m_terminal,
                Row { prompt, inputEdit },
            }.emerge();
            // clang-format on

            inputEdit->setReadOnly(true);
            inputEdit->setHistoryCompleter(Utils::Key("LuaREPL.InputHistory"));

            connect(inputEdit, &QLineEdit::returnPressed, this, [this, inputEdit] {
                inputEdit->setReadOnly(true);
                m_terminal->handleRequestResult(inputEdit->text());
                inputEdit->onEditingFinished();
                inputEdit->clear();
            });
            connect(
                m_terminal,
                &LuaTerminal::inputRequested,
                this,
                [prompt, inputEdit](const QString &p) {
                    prompt->setText(p);
                    inputEdit->setReadOnly(false);
                });
            connect(m_terminal, &LuaTerminal::keyPressed, this, [inputEdit](QKeyEvent *e) {
                inputEdit->keyPressEvent(e);
                inputEdit->setFocus();
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
    std::unique_ptr<LuaEngine> m_luaEngine;
    LuaPane *m_pane = nullptr;

public:
    LuaPlugin() {}
    ~LuaPlugin() override = default;

    void initialize() final
    {
        m_luaEngine.reset(new LuaEngine());

        addAsyncModule();
        addFetchModule();
        addActionModule();
        addUtilsModule();
        addMessageManagerModule();
        addProcessModule();
        addSettingsModule();
        addGuiModule();
        addQtModule();
        addCoreModule();
        addHookModule();
        addInstallModule();

        Core::JsExpander::registerGlobalObject("Lua", [] { return new LuaJsExtension(); });

        m_pane = new LuaPane;
        ExtensionSystem::PluginManager::addObject(m_pane);
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

                const expected_str<LuaPluginSpec *> result = m_luaEngine->loadPlugin(script);

                if (!result) {
                    qWarning() << "Failed to load plugin" << script << ":" << result.error();
                    MessageManager::writeFlashing(tr("Failed to load plugin %1: %2")
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
