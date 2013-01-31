/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppcodestylepreferences.h"
#include "cppcodestylesettingspage.h"
#include "cpppointerdeclarationformatter.h"
#include "cppqtstyleindenter.h"
#include "cpptoolsconstants.h"
#include "cpptoolssettings.h"
#include "ui_cppcodestylesettingspage.h"

#include <Overview.h>
#include <pp.h>

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

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

static void applyRefactorings(QTextDocument *textDocument, TextEditor::BaseTextEditorWidget *editor,
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

    TextEditor::TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    decorateEditors(textEditorSettings->fontSettings());
    connect(textEditorSettings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(decorateEditors(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    connect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
       this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    connect(m_ui->indentBlockBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentBlockBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentClassBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentEnumBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentSwitchLabels, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseStatements, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBlocks, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBreak, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentDeclarationsRelativeToAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->extraPaddingConditions, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->alignAssignments, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->bindStarToIdentifier, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->bindStarToTypeName, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->bindStarToLeftSpecifier, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->bindStarToRightSpecifier, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));

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

    connect(m_preferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
            this, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(m_preferences, SIGNAL(currentCodeStyleSettingsChanged(CppTools::CppCodeStyleSettings)),
            this, SLOT(setCodeStyleSettings(CppTools::CppCodeStyleSettings)));
    connect(m_preferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
            this, SLOT(slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences*)));

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

    return set;
}

void CppCodeStylePreferencesWidget::setTabSettings(const TextEditor::TabSettings &settings)
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
    m_blockUpdates = wasBlocked;
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences, bool preview)
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

QString CppCodeStylePreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
       << sep << m_ui->tabSettingsWidget->searchKeywords()
       << sep << m_ui->indentBlockBraces->text()
       << sep << m_ui->indentBlockBody->text()
       << sep << m_ui->indentClassBraces->text()
       << sep << m_ui->indentEnumBraces->text()
       << sep << m_ui->indentNamespaceBraces->text()
       << sep << m_ui->indentNamespaceBody->text()
       << sep << m_ui->indentAccessSpecifiers->text()
       << sep << m_ui->indentDeclarationsRelativeToAccessSpecifiers->text()
       << sep << m_ui->indentFunctionBody->text()
       << sep << m_ui->indentFunctionBraces->text()
       << sep << m_ui->indentSwitchLabels->text()
       << sep << m_ui->indentCaseStatements->text()
       << sep << m_ui->indentCaseBlocks->text()
       << sep << m_ui->indentCaseBreak->text()
       << sep << m_ui->bindStarToIdentifier->text()
       << sep << m_ui->bindStarToTypeName->text()
       << sep << m_ui->bindStarToLeftSpecifier->text()
       << sep << m_ui->bindStarToRightSpecifier->text()
       << sep << m_ui->contentGroupBox->title()
       << sep << m_ui->bracesGroupBox->title()
       << sep << m_ui->switchGroupBox->title()
       << sep << m_ui->alignmentGroupBox->title()
       << sep << m_ui->pointerReferencesGroupBox->title()
       << sep << m_ui->extraPaddingConditions->text()
       << sep << m_ui->alignAssignments->text()
          ;
    for (int i = 0; i < m_ui->categoryTab->count(); i++)
        QTextStream(&rc) << sep << m_ui->categoryTab->tabText(i);
    rc.remove(QLatin1Char('&'));
    return rc;
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

void CppCodeStylePreferencesWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
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
    const TextEditor::TabSettings ts = cppCodeStylePreferences->currentTabSettings();
    QtStyleCodeFormatter formatter(ts, ccss);
    foreach (TextEditor::SnippetEditorWidget *preview, m_previews) {
        preview->setTabSettings(ts);
        preview->setCodeStyle(cppCodeStylePreferences);

        QTextDocument *doc = preview->document();
        formatter.invalidateCache(doc);

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->indenter()->indentBlock(doc, block, QChar::Null, ts);

            block = block.next();
        }
        applyRefactorings(doc, preview, ccss);
        tc.endEditBlock();
    }
}

void CppCodeStylePreferencesWidget::decorateEditors(const TextEditor::FontSettings &fontSettings)
{
    const ISnippetProvider *provider = 0;
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<ISnippetProvider>();
    foreach (const ISnippetProvider *current, providers) {
        if (current->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID)) {
            provider = current;
            break;
        }
    }

    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
        editor->setFontSettings(fontSettings);
        if (provider)
            provider->decorateEditor(editor);
    }
}

void CppCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
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
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeStyleSettingsPage::createPage(QWidget *parent)
{
    CppCodeStylePreferences *originalCodeStylePreferences
            = CppToolsSettings::instance()->cppCodeStyle();
    m_pageCppCodeStylePreferences = new CppCodeStylePreferences(m_widget);
    m_pageCppCodeStylePreferences->setDelegatingPool(originalCodeStylePreferences->delegatingPool());
    m_pageCppCodeStylePreferences->setCodeStyleSettings(originalCodeStylePreferences->codeStyleSettings());
    m_pageCppCodeStylePreferences->setCurrentDelegate(originalCodeStylePreferences->currentDelegate());
    // we set id so that it won't be possible to set delegate to the original prefs
    m_pageCppCodeStylePreferences->setId(originalCodeStylePreferences->id());
    TextEditorSettings *settings = TextEditorSettings::instance();
    m_widget = new CodeStyleEditor(settings->codeStyleFactory(CppTools::Constants::CPP_SETTINGS_ID),
                                   m_pageCppCodeStylePreferences, parent);

    return m_widget;
}

void CppCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::settings();

        CppCodeStylePreferences *originalCppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
        if (originalCppCodeStylePreferences->codeStyleSettings() != m_pageCppCodeStylePreferences->codeStyleSettings()) {
            originalCppCodeStylePreferences->setCodeStyleSettings(m_pageCppCodeStylePreferences->codeStyleSettings());
            if (s)
                originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->tabSettings() != m_pageCppCodeStylePreferences->tabSettings()) {
            originalCppCodeStylePreferences->setTabSettings(m_pageCppCodeStylePreferences->tabSettings());
            if (s)
                originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->currentDelegate() != m_pageCppCodeStylePreferences->currentDelegate()) {
            originalCppCodeStylePreferences->setCurrentDelegate(m_pageCppCodeStylePreferences->currentDelegate());
            if (s)
                originalCppCodeStylePreferences->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
    }
}

bool CppCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace CppTools
