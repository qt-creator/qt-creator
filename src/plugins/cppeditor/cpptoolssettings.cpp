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
#include <texteditor/codestyleselectorwidget.h>
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

CppCodeStylePreferences *cppCodeStyle()
{
    // The C++ factory registers the global style; ClangFormat replaces the
    // factory (same language id) but reuses that same registered global.
    return static_cast<CppCodeStylePreferences *>(
        codeStyleForLanguage(Constants::CPP_SETTINGS_ID));
}

class CppCodeStyleEditor final : public CodeStyleEditor
{
public:
    explicit CppCodeStyleEditor(CppCodeStylePreferences *codeStyle)
        : m_selector{{}, this}
        , m_widget{codeStyle}
    {
        m_selector.setCodeStyle(codeStyle);
        addSelector(&m_selector);
        m_widget.layout()->setContentsMargins(0, 0, 0, 0);
        addEditorWidget(&m_widget);
        // The widget edits the (page-local) style live; report changes so the
        // hosting CodeStyleAspect can track dirtiness and defer the commit.
        connect(codeStyle, &ICodeStylePreferences::currentValueChanged,
                this, &CodeStyleEditor::changed);
        connect(codeStyle, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &CodeStyleEditor::changed);
    }

private:
    CodeStyleSelectorWidget m_selector;
    CppCodeStylePreferencesWidget m_widget;
};

class CppCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory()
        : ICodeStylePreferencesFactory(Constants::CPP_SETTINGS_ID)
    {
        qRegisterMetaType<CppCodeStyleSettings>("CppEditor::CppCodeStyleSettings");

        setDisplayName(Tr::tr(Constants::CPP_SETTINGS_NAME));
        setSnippetGroupId(Constants::CPP_SNIPPETS_GROUP_ID);
        setPreviewText(QString::fromLatin1(Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]));
        setIndenterCreator([](QTextDocument *doc) { return createCppQtStyleIndenter(doc); });
        setCodeStyleCreator([] { return new CppCodeStylePreferences; });
        setSettingsEditorCreator([](ICodeStylePreferences *codeStyle) {
            return new CppCodeStyleEditor{static_cast<CppCodeStylePreferences *>(codeStyle)};
        });

        setGlobalCodeStyleId(idKey);
        setDefaultCodeStyleId("qt");
        setBuiltInCodeStyles([this](CodeStylePool *pool) {
            // Qt style
            m_qtCodeStyle.setId("qt");
            m_qtCodeStyle.setDisplayName(Tr::tr("Qt"));
            m_qtCodeStyle.setReadOnly(true);
            TabSettingsData qtTabSettings;
            qtTabSettings.m_tabPolicy = TabSettingsData::SpacesOnlyTabPolicy;
            qtTabSettings.m_tabSize = 4;
            qtTabSettings.m_indentSize = 4;
            qtTabSettings.m_continuationAlignBehavior = TabSettingsData::ContinuationAlignWithIndent;
            qtTabSettings.m_autoDetect = false;
            m_qtCodeStyle.setTabSettings(qtTabSettings);
            pool->addCodeStyle(&m_qtCodeStyle);

            // GNU style
            m_gnuCodeStyle.setId("gnu");
            m_gnuCodeStyle.setDisplayName(Tr::tr("GNU"));
            m_gnuCodeStyle.setReadOnly(true);
            TabSettingsData gnuTabSettings;
            gnuTabSettings.m_tabPolicy = TabSettingsData::TabsOnlyTabPolicy;
            gnuTabSettings.m_tabSize = 2;
            gnuTabSettings.m_indentSize = 2;
            gnuTabSettings.m_autoDetect = false;
            gnuTabSettings.m_continuationAlignBehavior = TabSettingsData::ContinuationAlignWithIndent;
            m_gnuCodeStyle.setTabSettings(gnuTabSettings);
            CppCodeStyleSettings gnuCodeStyleSettings;
            gnuCodeStyleSettings.indentNamespaceBody = true;
            gnuCodeStyleSettings.indentBlockBraces = true;
            gnuCodeStyleSettings.indentSwitchLabels = true;
            gnuCodeStyleSettings.indentBlocksRelativeToSwitchLabels = true;
            m_gnuCodeStyle.setCodeStyleSettings(gnuCodeStyleSettings);
            pool->addCodeStyle(&m_gnuCodeStyle);
        });
        setupCodeStyles();

        using namespace Utils::Constants;
        registerMimeTypeForLanguageId(C_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
        registerMimeTypeForLanguageId(C_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
        registerMimeTypeForLanguageId(CPP_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
        registerMimeTypeForLanguageId(CPP_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
    }

private:
    CppCodeStylePreferences m_qtCodeStyle;
    CppCodeStylePreferences m_gnuCodeStyle;
};

void Internal::setupCppToolsSettings()
{
    static GuardedObject<CppCodeStylePreferencesFactory> theCppCodeStylePreferencesFactory;
}

} // namespace CppEditor
