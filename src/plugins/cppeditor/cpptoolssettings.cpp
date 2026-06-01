// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpptoolssettings.h"

#include "cppcodestylesettings.h"
#include "cppcodestylesettingspage.h"
#include "cppcodestylesnippets.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppqtstyleindenter.h"

#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/completionsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepool.h>

#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>

#include <QLayout>

static const char idKey[] = "CppGlobal";

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

static CppCodeStylePreferences *g_globalCodeStyle = nullptr;

CppCodeStylePreferences *cppCodeStyle()
{
    return g_globalCodeStyle;
}

class CppCodeStyleEditor final : public CodeStyleEditor
{
public:
    CppCodeStyleEditor(QWidget *parent)
        : CodeStyleEditor{parent}
    {}

private:
    CodeStyleWidget *createEditorWidget(
        const FilePath & /*projectFile*/,
        ICodeStylePreferences *codeStyle,
        QWidget *parent) const final
    {
        auto cppPreferences = dynamic_cast<CppCodeStylePreferences *>(codeStyle);
        if (cppPreferences == nullptr)
            return nullptr;

        auto widget = new CppCodeStylePreferencesWidget(parent);
        widget->layout()->setContentsMargins(0, 0, 0, 0);
        widget->setCodeStyle(cppPreferences);
        return widget;
    }

    QString previewText() const final
    {
        return QString::fromLatin1(Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]);
    }

    QString snippetProviderGroupId() const final
    {
        return Constants::CPP_SNIPPETS_GROUP_ID;
    }
};

class CppCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(Constants::CPP_SETTINGS_ID)
    {}

private:
    CodeStyleEditor *createCodeStyleEditor(
            const FilePath &projectFile,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const final
    {
        auto editor = new CppCodeStyleEditor{parent};
        editor->init(this, projectFile, codeStyle);
        return editor;
    }

    QString displayName() final { return Tr::tr(Constants::CPP_SETTINGS_NAME); }

    ICodeStylePreferences *createCodeStyle() const final
    {
        return new CppCodeStylePreferences;
    }

    Indenter *createIndenter(QTextDocument *doc) const final
    {
        return createCppQtStyleIndenter(doc);
    }
};

class CppToolsSettings final : public QObject
{
public:
    CppToolsSettings();
    ~CppToolsSettings() final;

    CppCodeStylePreferencesFactory m_factory;
    CodeStylePool m_pool{&m_factory, Constants::CPP_SETTINGS_ID};
};

CppToolsSettings::CppToolsSettings()
{
    qRegisterMetaType<CppCodeStyleSettings>("CppEditor::CppCodeStyleSettings");

    // global code style settings
    g_globalCodeStyle = new CppCodeStylePreferences(this);
    g_globalCodeStyle->setDelegatingPool(&m_pool);
    g_globalCodeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    g_globalCodeStyle->setId(idKey);
    m_pool.addCodeStyle(g_globalCodeStyle);
    registerCodeStyle(Constants::CPP_SETTINGS_ID, g_globalCodeStyle);

    /*
    For every language we have exactly 1 pool. The pool contains:
    1) All built-in code styles (Qt/GNU)
    2) All custom code styles (which will be added dynamically)
    3) A global code style

    If the code style gets a pool (setCodeStylePool()) it means it can behave
    like a proxy to one of the code styles from that pool
    (ICodeStylePreferences::setCurrentDelegate()).
    That's why the global code style gets a pool (it can point to any code style
    from the pool), while built-in and custom code styles don't get a pool
    (they can't point to any other code style).

    The instance of the language pool is shared. The same instance of the pool
    is used for all project code style settings and for global one.
    Project code style can point to one of built-in or custom code styles
    or to the global one as well. That's why the global code style is added
    to the pool. The proxy chain can look like:
    ProjectCodeStyle -> GlobalCodeStyle -> BuildInCodeStyle (e.g. Qt).

    With the global pool there is an exception - it gets a pool
    in which it exists itself. The case in which a code style point to itself
    is disallowed and is handled in ICodeStylePreferences::setCurrentDelegate().
    */

    // built-in settings
    // Qt style
    auto qtCodeStyle = new CppCodeStylePreferences(this);
    qtCodeStyle->setId("qt");
    qtCodeStyle->setDisplayName(Tr::tr("Qt"));
    qtCodeStyle->setReadOnly(true);
    TabSettingsData qtTabSettings;
    qtTabSettings.m_tabPolicy = TabSettingsData::SpacesOnlyTabPolicy;
    qtTabSettings.m_tabSize = 4;
    qtTabSettings.m_indentSize = 4;
    qtTabSettings.m_continuationAlignBehavior = TabSettingsData::ContinuationAlignWithIndent;
    qtTabSettings.m_autoDetect = false;
    qtCodeStyle->setTabSettings(qtTabSettings);
    m_pool.addCodeStyle(qtCodeStyle);

    // GNU style
    auto gnuCodeStyle = new CppCodeStylePreferences(this);
    gnuCodeStyle->setId("gnu");
    gnuCodeStyle->setDisplayName(Tr::tr("GNU"));
    gnuCodeStyle->setReadOnly(true);
    TabSettingsData gnuTabSettings;
    gnuTabSettings.m_tabPolicy = TabSettingsData::TabsOnlyTabPolicy;
    gnuTabSettings.m_tabSize = 2;
    gnuTabSettings.m_indentSize = 2;
    gnuTabSettings.m_autoDetect = false;
    gnuTabSettings.m_continuationAlignBehavior = TabSettingsData::ContinuationAlignWithIndent;
    gnuCodeStyle->setTabSettings(gnuTabSettings);
    CppCodeStyleSettings gnuCodeStyleSettings;
    gnuCodeStyleSettings.indentNamespaceBody = true;
    gnuCodeStyleSettings.indentBlockBraces = true;
    gnuCodeStyleSettings.indentSwitchLabels = true;
    gnuCodeStyleSettings.indentBlocksRelativeToSwitchLabels = true;
    gnuCodeStyle->setCodeStyleSettings(gnuCodeStyleSettings);
    m_pool.addCodeStyle(gnuCodeStyle);

    // default delegate for global preferences
    g_globalCodeStyle->setCurrentDelegate(qtCodeStyle);

    m_pool.loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    g_globalCodeStyle->fromSettings(Constants::CPP_SETTINGS_ID);

    // mimetypes to be handled
    using namespace Utils::Constants;
    registerMimeTypeForLanguageId(C_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
    registerMimeTypeForLanguageId(C_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
    registerMimeTypeForLanguageId(CPP_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
    registerMimeTypeForLanguageId(CPP_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
}

CppToolsSettings::~CppToolsSettings()
{
    unregisterCodeStyle(Constants::CPP_SETTINGS_ID);

    delete g_globalCodeStyle;
    g_globalCodeStyle = nullptr;
}

void Internal::setupCppToolsSettings()
{
    static GuardedObject<CppToolsSettings> theCppToolsSettings;
}

} // namespace CppEditor
