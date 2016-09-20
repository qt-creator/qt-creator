/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppcodestylesettingspage.h"

#include "cppcodestylepreferences.h"
#include "cpppointerdeclarationformatter.h"
#include "cppqtstyleindenter.h"
#include "cpptoolsconstants.h"
#include "cpptoolssettings.h"
#include <ui_cppcodestylesettingspage.h>

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/displaysettings.h>
#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/texteditorsettings.h>

#include <cplusplus/Overview.h>
#include <cplusplus/pp.h>

#include <extensionsystem/pluginmanager.h>

#include <QTextBlock>
#include <QTextStream>

static const char *defaultCodeStyleSnippets[] = {
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "namespace Foo\n"
    "{\n"
    "namespace Bar\n"
    "{\n"
    "class FooBar\n"
    "    {\n"
    "public:\n"
    "    FooBar(int a)\n"
    "        : _a(a)\n"
    "        {}\n"
    "    int calculate() const\n"
    "        {\n"
    "        if (a > 10)\n"
    "            {\n"
    "            int b = 2 * a;\n"
    "            return a * b;\n"
    "            }\n"
    "        return -a;\n"
    "        }\n"
    "private:\n"
    "    int _a;\n"
    "    };\n"
    "}\n"
    "}\n"
    ,
    "#include \"bar.h\"\n"
    "\n"
    "int foo(int a)\n"
    "    {\n"
    "    switch (a)\n"
    "        {\n"
    "        case 1:\n"
    "            bar(1);\n"
    "            break;\n"
    "        case 2:\n"
    "            {\n"
    "            bar(2);\n"
    "            break;\n"
    "            }\n"
    "        case 3:\n"
    "        default:\n"
    "            bar(3);\n"
    "            break;\n"
    "        }\n"
    "    return 0;\n"
    "    }\n"
    ,
    "void foo() {\n"
    "    if (a &&\n"
    "        b)\n"
    "        c;\n"
    "\n"
    "    while (a ||\n"
    "           b)\n"
    "        break;\n"
    "    a = b +\n"
    "        c;\n"
    "    myInstance.longMemberName +=\n"
    "            foo;\n"
    "    myInstance.longMemberName += bar +\n"
    "                                 foo;\n"
    "}\n"
    ,
    "int *foo(const Bar &b1, Bar &&b2, int*, int *&rpi)\n"
    "{\n"
    "    int*pi = 0;\n"
    "    int*const*const cpcpi = &pi;\n"
    "    int*const*pcpi = &pi;\n"
    "    int**const cppi = &pi;\n"
    "\n"
    "    void (*foo)(char *s) = 0;\n"
    "    int (*bar)[] = 0;\n"
    "\n"
    "    return pi;\n"
    "}\n"
};

using namespace TextEditor;

namespace CppTools {

namespace Internal {

static void applyRefactorings(QTextDocument *textDocument, TextEditorWidget *editor,
                              const CppCodeStyleSettings &settings)
{
    // Preprocess source
    Environment env;
    Preprocessor preprocess(0, &env);
    const QByteArray preprocessedSource
        = preprocess.run(QLatin1String("<no-file>"), textDocument->toPlainText());

    Document::Ptr cppDocument = Document::create(QLatin1String("<no-file>"));
    cppDocument->setUtf8Source(preprocessedSource);
    cppDocument->parse(Document::ParseTranlationUnit);
    cppDocument->check();

    CppRefactoringFilePtr cppRefactoringFile = CppRefactoringChanges::file(editor, cppDocument);

    // Run the formatter
    Overview overview;
    overview.showReturnTypes = true;
    overview.starBindFlags = Overview::StarBindFlags(0);

    if (settings.bindStarToIdentifier)
        overview.starBindFlags |= Overview::BindToIdentifier;
    if (settings.bindStarToTypeName)
        overview.starBindFlags |= Overview::BindToTypeName;
    if (settings.bindStarToLeftSpecifier)
        overview.starBindFlags |= Overview::BindToLeftSpecifier;
    if (settings.bindStarToRightSpecifier)
        overview.starBindFlags |= Overview::BindToRightSpecifier;

    PointerDeclarationFormatter formatter(cppRefactoringFile, overview);
    Utils::ChangeSet change = formatter.format(cppDocument->translationUnit()->ast());

    // Apply change
    QTextCursor cursor(textDocument);
    change.apply(&cursor);
}

// ------------------ CppCodeStyleSettingsWidget

CppCodeStylePreferencesWidget::CppCodeStylePreferencesWidget(QWidget *parent)
    : QWidget(parent),
      m_preferences(0),
      m_ui(new Ui::CppCodeStyleSettingsPage),
      m_blockUpdates(false)
{
    m_ui->setupUi(this);
    m_ui->categoryTab->setProperty("_q_custom_style_disabled", true);

    m_previews << m_ui->previewTextEditGeneral << m_ui->previewTextEditContent
               << m_ui->previewTextEditBraces << m_ui->previewTextEditSwitch
               << m_ui->previewTextEditPadding << m_ui->previewTextEditPointerReferences;
    for (int i = 0; i < m_previews.size(); ++i)
        m_previews[i]->setPlainText(QLatin1String(defaultCodeStyleSnippets[i]));

    decorateEditors(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &CppCodeStylePreferencesWidget::decorateEditors);

    setVisualizeWhitespace(true);

    connect(m_ui->tabSettingsWidget, &TabSettingsWidget::settingsChanged,
            this, &CppCodeStylePreferencesWidget::slotTabSettingsChanged);
    connect(m_ui->indentBlockBraces, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentBlockBody, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentClassBraces, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentNamespaceBraces, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentEnumBraces, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentNamespaceBody, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentSwitchLabels, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentCaseStatements, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentCaseBlocks, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentCaseBreak, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentAccessSpecifiers, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentDeclarationsRelativeToAccessSpecifiers, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentFunctionBody, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->indentFunctionBraces, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->extraPaddingConditions, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->alignAssignments, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->bindStarToIdentifier, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->bindStarToTypeName, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->bindStarToLeftSpecifier, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->bindStarToRightSpecifier, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
    connect(m_ui->preferGetterNamesWithoutGet, &QCheckBox::toggled,
            this, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);

    m_ui->categoryTab->setCurrentIndex(0);

    m_ui->tabSettingsWidget->setFlat(true);
}

CppCodeStylePreferencesWidget::~CppCodeStylePreferencesWidget()
{
    delete m_ui;
}

void CppCodeStylePreferencesWidget::setCodeStyle(CppTools::CppCodeStylePreferences *codeStylePreferences)
{
    // code preferences
    m_preferences = codeStylePreferences;

    connect(m_preferences, &CppCodeStylePreferences::currentTabSettingsChanged,
            this, &CppCodeStylePreferencesWidget::setTabSettings);
    connect(m_preferences, &CppCodeStylePreferences::currentCodeStyleSettingsChanged,
            this, [this](const CppTools::CppCodeStyleSettings &codeStyleSettings) {
        setCodeStyleSettings(codeStyleSettings);
    });

    connect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
            this, [this](TextEditor::ICodeStylePreferences *currentPreferences) {
        slotCurrentPreferencesChanged(currentPreferences);
    });

    setTabSettings(m_preferences->tabSettings());
    setCodeStyleSettings(m_preferences->codeStyleSettings(), false);
    slotCurrentPreferencesChanged(m_preferences->currentPreferences(), false);

    updatePreview();
}

CppCodeStyleSettings CppCodeStylePreferencesWidget::cppCodeStyleSettings() const
{
    CppCodeStyleSettings set;

    set.indentBlockBraces = m_ui->indentBlockBraces->isChecked();
    set.indentBlockBody = m_ui->indentBlockBody->isChecked();
    set.indentClassBraces = m_ui->indentClassBraces->isChecked();
    set.indentEnumBraces = m_ui->indentEnumBraces->isChecked();
    set.indentNamespaceBraces = m_ui->indentNamespaceBraces->isChecked();
    set.indentNamespaceBody = m_ui->indentNamespaceBody->isChecked();
    set.indentAccessSpecifiers = m_ui->indentAccessSpecifiers->isChecked();
    set.indentDeclarationsRelativeToAccessSpecifiers = m_ui->indentDeclarationsRelativeToAccessSpecifiers->isChecked();
    set.indentFunctionBody = m_ui->indentFunctionBody->isChecked();
    set.indentFunctionBraces = m_ui->indentFunctionBraces->isChecked();
    set.indentSwitchLabels = m_ui->indentSwitchLabels->isChecked();
    set.indentStatementsRelativeToSwitchLabels = m_ui->indentCaseStatements->isChecked();
    set.indentBlocksRelativeToSwitchLabels = m_ui->indentCaseBlocks->isChecked();
    set.indentControlFlowRelativeToSwitchLabels = m_ui->indentCaseBreak->isChecked();
    set.bindStarToIdentifier = m_ui->bindStarToIdentifier->isChecked();
    set.bindStarToTypeName = m_ui->bindStarToTypeName->isChecked();
    set.bindStarToLeftSpecifier = m_ui->bindStarToLeftSpecifier->isChecked();
    set.bindStarToRightSpecifier = m_ui->bindStarToRightSpecifier->isChecked();
    set.extraPaddingForConditionsIfConfusingAlign = m_ui->extraPaddingConditions->isChecked();
    set.alignAssignments = m_ui->alignAssignments->isChecked();
    set.preferGetterNameWithoutGetPrefix = m_ui->preferGetterNamesWithoutGet->isChecked();

    return set;
}

void CppCodeStylePreferencesWidget::setTabSettings(const TabSettings &settings)
{
    m_ui->tabSettingsWidget->setTabSettings(settings);
}

void CppCodeStylePreferencesWidget::setCodeStyleSettings(const CppCodeStyleSettings &s, bool preview)
{
    const bool wasBlocked = m_blockUpdates;
    m_blockUpdates = true;
    m_ui->indentBlockBraces->setChecked(s.indentBlockBraces);
    m_ui->indentBlockBody->setChecked(s.indentBlockBody);
    m_ui->indentClassBraces->setChecked(s.indentClassBraces);
    m_ui->indentEnumBraces->setChecked(s.indentEnumBraces);
    m_ui->indentNamespaceBraces->setChecked(s.indentNamespaceBraces);
    m_ui->indentNamespaceBody->setChecked(s.indentNamespaceBody);
    m_ui->indentAccessSpecifiers->setChecked(s.indentAccessSpecifiers);
    m_ui->indentDeclarationsRelativeToAccessSpecifiers->setChecked(s.indentDeclarationsRelativeToAccessSpecifiers);
    m_ui->indentFunctionBody->setChecked(s.indentFunctionBody);
    m_ui->indentFunctionBraces->setChecked(s.indentFunctionBraces);
    m_ui->indentSwitchLabels->setChecked(s.indentSwitchLabels);
    m_ui->indentCaseStatements->setChecked(s.indentStatementsRelativeToSwitchLabels);
    m_ui->indentCaseBlocks->setChecked(s.indentBlocksRelativeToSwitchLabels);
    m_ui->indentCaseBreak->setChecked(s.indentControlFlowRelativeToSwitchLabels);
    m_ui->bindStarToIdentifier->setChecked(s.bindStarToIdentifier);
    m_ui->bindStarToTypeName->setChecked(s.bindStarToTypeName);
    m_ui->bindStarToLeftSpecifier->setChecked(s.bindStarToLeftSpecifier);
    m_ui->bindStarToRightSpecifier->setChecked(s.bindStarToRightSpecifier);
    m_ui->extraPaddingConditions->setChecked(s.extraPaddingForConditionsIfConfusingAlign);
    m_ui->alignAssignments->setChecked(s.alignAssignments);
    m_ui->preferGetterNamesWithoutGet->setChecked(s.preferGetterNameWithoutGetPrefix);
    m_blockUpdates = wasBlocked;
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCurrentPreferencesChanged(ICodeStylePreferences *preferences, bool preview)
{
    const bool enable = !preferences->isReadOnly() && !m_preferences->currentDelegate();
    m_ui->tabSettingsWidget->setEnabled(enable);
    m_ui->contentGroupBox->setEnabled(enable);
    m_ui->bracesGroupBox->setEnabled(enable);
    m_ui->switchGroupBox->setEnabled(enable);
    m_ui->alignmentGroupBox->setEnabled(enable);
    m_ui->pointerReferencesGroupBox->setEnabled(enable);
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged()
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        CppCodeStylePreferences *current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setCodeStyleSettings(cppCodeStyleSettings());
    }

    updatePreview();
}

void CppCodeStylePreferencesWidget::slotTabSettingsChanged(const TabSettings &settings)
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        CppCodeStylePreferences *current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setTabSettings(settings);
    }

    updatePreview();
}

void CppCodeStylePreferencesWidget::updatePreview()
{
    CppCodeStylePreferences *cppCodeStylePreferences = m_preferences
            ? m_preferences
            : CppToolsSettings::instance()->cppCodeStyle();
    const CppCodeStyleSettings ccss = cppCodeStylePreferences->currentCodeStyleSettings();
    const TabSettings ts = cppCodeStylePreferences->currentTabSettings();
    QtStyleCodeFormatter formatter(ts, ccss);
    foreach (SnippetEditorWidget *preview, m_previews) {
        preview->textDocument()->setTabSettings(ts);
        preview->setCodeStyle(cppCodeStylePreferences);

        QTextDocument *doc = preview->document();
        formatter.invalidateCache(doc);

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->textDocument()->indenter()->indentBlock(doc, block, QChar::Null, ts);

            block = block.next();
        }
        applyRefactorings(doc, preview, ccss);
        tc.endEditBlock();
    }
}

void CppCodeStylePreferencesWidget::decorateEditors(const FontSettings &fontSettings)
{
    const ISnippetProvider *provider = ExtensionSystem::PluginManager::getObject<ISnippetProvider>(
        [](ISnippetProvider *current) {
            return current->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
        });

    foreach (SnippetEditorWidget *editor, m_previews) {
        editor->textDocument()->setFontSettings(fontSettings);
        if (provider)
            provider->decorateEditor(editor);
    }
}

void CppCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    foreach (SnippetEditorWidget *editor, m_previews) {
        DisplaySettings displaySettings = editor->displaySettings();
        displaySettings.m_visualizeWhitespace = on;
        editor->setDisplaySettings(displaySettings);
    }
}


// ------------------ CppCodeStyleSettingsPage

CppCodeStyleSettingsPage::CppCodeStyleSettingsPage(QWidget *parent) :
    Core::IOptionsPage(parent),
    m_pageCppCodeStylePreferences(0)
{
    setId(Constants::CPP_CODE_STYLE_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppTools", Constants::CPP_CODE_STYLE_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeStyleSettingsPage::widget()
{
    if (!m_widget) {
        CppCodeStylePreferences *originalCodeStylePreferences
                = CppToolsSettings::instance()->cppCodeStyle();
        m_pageCppCodeStylePreferences = new CppCodeStylePreferences(m_widget);
        m_pageCppCodeStylePreferences->setDelegatingPool(originalCodeStylePreferences->delegatingPool());
        m_pageCppCodeStylePreferences->setCodeStyleSettings(originalCodeStylePreferences->codeStyleSettings());
        m_pageCppCodeStylePreferences->setCurrentDelegate(originalCodeStylePreferences->currentDelegate());
        // we set id so that it won't be possible to set delegate to the original prefs
        m_pageCppCodeStylePreferences->setId(originalCodeStylePreferences->id());
        m_widget = new CodeStyleEditor(TextEditorSettings::codeStyleFactory(CppTools::Constants::CPP_SETTINGS_ID),
                                       m_pageCppCodeStylePreferences);
    }
    return m_widget;
}

void CppCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::settings();

        CppCodeStylePreferences *originalCppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
        if (originalCppCodeStylePreferences->codeStyleSettings() != m_pageCppCodeStylePreferences->codeStyleSettings()) {
            originalCppCodeStylePreferences->setCodeStyleSettings(m_pageCppCodeStylePreferences->codeStyleSettings());
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->tabSettings() != m_pageCppCodeStylePreferences->tabSettings()) {
            originalCppCodeStylePreferences->setTabSettings(m_pageCppCodeStylePreferences->tabSettings());
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->currentDelegate() != m_pageCppCodeStylePreferences->currentDelegate()) {
            originalCppCodeStylePreferences->setCurrentDelegate(m_pageCppCodeStylePreferences->currentDelegate());
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
    }
}

void CppCodeStyleSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace CppTools
