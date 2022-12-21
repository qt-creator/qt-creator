// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodestylesettingspage.h"

#include "cppcodeformatter.h"
#include "cppcodestylepreferences.h"
#include "cppcodestylesnippets.h"
#include "cppeditorconstants.h"
#include "cpppointerdeclarationformatter.h"
#include "cpptoolssettings.h"

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/displaysettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/snippets/snippeteditor.h>
#include <texteditor/tabsettings.h>
#include <texteditor/tabsettingswidget.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <cplusplus/Overview.h>
#include <cplusplus/pp.h>

#include <extensionsystem/pluginmanager.h>

#include <QCheckBox>
#include <QTabWidget>
#include <QTextBlock>

using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {

static void applyRefactorings(QTextDocument *textDocument, TextEditorWidget *editor,
                              const CppCodeStyleSettings &settings)
{
    // Preprocess source
    CPlusPlus::Environment env;
    Preprocessor preprocess(nullptr, &env);
    FilePath noFileFile = FilePath::fromPathPart(u"<no-file>");
    const QByteArray preprocessedSource
        = preprocess.run(noFileFile, textDocument->toPlainText().toUtf8());

    Document::Ptr cppDocument = Document::create(noFileFile);
    cppDocument->setUtf8Source(preprocessedSource);
    cppDocument->parse(Document::ParseTranlationUnit);
    cppDocument->check();

    CppRefactoringFilePtr cppRefactoringFile = CppRefactoringChanges::file(editor, cppDocument);

    // Run the formatter
    Overview overview;
    overview.showReturnTypes = true;
    overview.starBindFlags = {};

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

class CppCodeStylePreferencesWidgetPrivate
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::CppCodeStyleSettingsPage)

public:
    CppCodeStylePreferencesWidgetPrivate(CppCodeStylePreferencesWidget *widget)
        : q(widget)
        , m_indentAccessSpecifiers(createCheckBox(tr("\"public\", \"protected\" and\n"
                                                     "\"private\" within class body")))
        , m_indentDeclarationsRelativeToAccessSpecifiers(
              createCheckBox(tr("Declarations relative to \"public\",\n"
                                "\"protected\" and \"private\"")))
        , m_indentFunctionBody(createCheckBox(tr("Statements within function body")))
        , m_indentBlockBody(createCheckBox(tr("Statements within blocks")))
        , m_indentNamespaceBody(createCheckBox(tr("Declarations within\n"
                                                  "\"namespace\" definition")))
        , m_indentClassBraces(createCheckBox(tr("Class declarations")))
        , m_indentNamespaceBraces(createCheckBox(tr("Namespace declarations")))
        , m_indentEnumBraces(createCheckBox(tr("Enum declarations")))
        , m_indentFunctionBraces(createCheckBox(tr("Function declarations")))
        , m_indentBlockBraces(createCheckBox(tr("Blocks")))
        , m_indentSwitchLabels(createCheckBox(tr("\"case\" or \"default\"")))
        , m_indentCaseStatements(createCheckBox(tr("Statements relative to\n"
                                                   "\"case\" or \"default\"")))
        , m_indentCaseBlocks(createCheckBox(tr("Blocks relative to\n"
                                               "\"case\" or \"default\"")))
        , m_indentCaseBreak(createCheckBox(tr("\"break\" statement relative to\n"
                                              "\"case\" or \"default\"")))
        , m_alignAssignments(createCheckBox(tr("Align after assignments"),
                                            tr("<html><head/><body>\n"
                                               "Enables alignment to tokens after =, += etc. When the option is disabled, regular continuation line indentation will be used.<br>\n"
                                               "<br>\n"
                                               "With alignment:\n"
                                               "<pre>\n"
                                               "a = a +\n"
                                               "    b\n"
                                               "</pre>\n"
                                               "Without alignment:\n"
                                               "<pre>\n"
                                               "a = a +\n"
                                               "        b\n"
                                               "</pre>\n"
                                               "</body></html>")))
        , m_extraPaddingConditions(createCheckBox(tr("Add extra padding to conditions\n"
                                                     "if they would align to the next line"),
                                                  tr("<html><head/><body>\n"
                                                     "Adds an extra level of indentation to multiline conditions in the switch, if, while and foreach statements if they would otherwise have the same or less indentation than a nested statement.\n"
                                                     "\n"
                                                     "For four-spaces indentation only if statement conditions are affected. Without extra padding:\n"
                                                     "<pre>\n"
                                                     "if (a &&\n"
                                                     "    b)\n"
                                                     "    c;\n"
                                                     "</pre>\n"
                                                     "With extra padding:\n"
                                                     "<pre>\n"
                                                     "if (a &&\n"
                                                     "        b)\n"
                                                     "    c;\n"
                                                     "</pre>\n"
                                                     "</body></html>")))
        , m_bindStarToIdentifier(createCheckBox(tr("Identifier"),
                                                tr("<html><head/><body>This does not apply to the star and reference symbol in pointer/reference to functions and arrays, e.g.:\n"
                                                   "<pre>   int (&rf)() = ...;\n"
                                                   "   int (*pf)() = ...;\n"
                                                   "\n"
                                                   "   int (&ra)[2] = ...;\n"
                                                   "   int (*pa)[2] = ...;\n"
                                                   "\n"
                                                   "</pre></body></html>")))
        , m_bindStarToTypeName(createCheckBox(tr("Type name")))
        , m_bindStarToLeftSpecifier(createCheckBox(tr("Left const/volatile")))
        , m_bindStarToRightSpecifier(createCheckBox(tr("Right const/volatile"),
                                                    tr("This does not apply to references.")))
        , m_categoryTab(new QTabWidget)
        , m_tabSettingsWidget(new TabSettingsWidget)
    {
        m_categoryTab->setProperty("_q_custom_style_disabled", true);

        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_tabSettingsWidget->sizePolicy().hasHeightForWidth());
        m_tabSettingsWidget->setSizePolicy(sizePolicy);
        m_tabSettingsWidget->setFocusPolicy(Qt::TabFocus);
        QObject::connect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                         q, &CppCodeStylePreferencesWidget::slotTabSettingsChanged);

        using namespace Utils::Layouting;

        QWidget *generalTab = new QWidget;
        Row {
            Column {
                m_tabSettingsWidget,
                st
            },
            createPreview(0)
        }.attachTo(generalTab);
        m_categoryTab->addTab(generalTab, tr("General"));
        m_controllers.append(m_tabSettingsWidget);

        QWidget *contentTab = new QWidget;
        Group contentGroup {
            title(tr("Indent")),
            Column {
                m_indentAccessSpecifiers,
                m_indentDeclarationsRelativeToAccessSpecifiers,
                m_indentFunctionBody,
                m_indentBlockBody,
                m_indentNamespaceBody,
                st
            }
        };
        Row {
            contentGroup,
            createPreview(1)
        }.attachTo(contentTab);
        m_categoryTab->addTab(contentTab, tr("Content"));
        m_controllers.append(contentGroup.widget);

        QWidget *bracesTab = new QWidget;
        Group bracesGroup {
            title(tr("Indent Braces")),
            Column {
                m_indentClassBraces,
                m_indentNamespaceBraces,
                m_indentEnumBraces,
                m_indentFunctionBraces,
                m_indentBlockBraces,
                st
            }
        };
        Row {
            bracesGroup,
            createPreview(2)
        }.attachTo(bracesTab);
        m_categoryTab->addTab(bracesTab, tr("Braces"));
        m_controllers.append(bracesGroup.widget);

        QWidget *switchTab = new QWidget;
        Group switchGroup {
            title(tr("Indent within \"switch\"")),
            Column {
                m_indentSwitchLabels,
                m_indentCaseStatements,
                m_indentCaseBlocks,
                m_indentCaseBreak,
                st
            }
        };
        Row {
            switchGroup,
            createPreview(3)
        }.attachTo(switchTab);
        m_categoryTab->addTab(switchTab, tr("\"switch\""));
        m_controllers.append(switchGroup.widget);

        QWidget *alignmentTab = new QWidget;
        Group alignmentGroup {
            title(tr("Align")),
            Column {
                m_alignAssignments,
                m_extraPaddingConditions,
                st
            }
        };
        Row {
            alignmentGroup,
            createPreview(4)
        }.attachTo(alignmentTab);
        m_categoryTab->addTab(alignmentTab, tr("Alignment"));
        m_controllers.append(alignmentGroup.widget);

        QWidget *typesTab = new QWidget;
        Group typesGroup {
            title(tr("Bind '*' and '&&' in types/declarations to")),
            Column {
                m_bindStarToIdentifier,
                m_bindStarToTypeName,
                m_bindStarToLeftSpecifier,
                m_bindStarToRightSpecifier,
                st
            }
        };
        Row {
            typesGroup,
            createPreview(5)
        }.attachTo(typesTab);
        m_categoryTab->addTab(typesTab, tr("Pointers and References"));
        m_controllers.append(typesGroup.widget);

        Row { m_categoryTab }.attachTo(q);
    }

    QCheckBox *createCheckBox(const QString &text, const QString &toolTip = {})
    {
        QCheckBox *checkBox = new QCheckBox(text);
        checkBox->setToolTip(toolTip);
        QObject::connect(checkBox, &QCheckBox::toggled,
                         q, &CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged);
        return checkBox;
    }

    SnippetEditorWidget *createPreview(int i)
    {
        SnippetEditorWidget *editor = new SnippetEditorWidget;
        editor->setPlainText(QLatin1String(Constants::DEFAULT_CODE_STYLE_SNIPPETS[i]));
        m_previews.append(editor);
        return editor;
    }

    CppCodeStylePreferencesWidget *q = nullptr;

    QCheckBox *m_indentAccessSpecifiers = nullptr;
    QCheckBox *m_indentDeclarationsRelativeToAccessSpecifiers = nullptr;
    QCheckBox *m_indentFunctionBody = nullptr;
    QCheckBox *m_indentBlockBody = nullptr;
    QCheckBox *m_indentNamespaceBody = nullptr;
    QCheckBox *m_indentClassBraces = nullptr;
    QCheckBox *m_indentNamespaceBraces = nullptr;
    QCheckBox *m_indentEnumBraces = nullptr;
    QCheckBox *m_indentFunctionBraces = nullptr;
    QCheckBox *m_indentBlockBraces = nullptr;
    QCheckBox *m_indentSwitchLabels = nullptr;
    QCheckBox *m_indentCaseStatements = nullptr;
    QCheckBox *m_indentCaseBlocks = nullptr;
    QCheckBox *m_indentCaseBreak = nullptr;
    QCheckBox *m_alignAssignments = nullptr;
    QCheckBox *m_extraPaddingConditions = nullptr;
    QCheckBox *m_bindStarToIdentifier = nullptr;
    QCheckBox *m_bindStarToTypeName = nullptr;
    QCheckBox *m_bindStarToLeftSpecifier = nullptr;
    QCheckBox *m_bindStarToRightSpecifier = nullptr;

    QList<SnippetEditorWidget *> m_previews;
    QList<QWidget *> m_controllers;

    QTabWidget *m_categoryTab = nullptr;
    TabSettingsWidget *m_tabSettingsWidget = nullptr;
};

CppCodeStylePreferencesWidget::CppCodeStylePreferencesWidget(QWidget *parent)
    : TextEditor::CodeStyleEditorWidget(parent),
      d(new CppCodeStylePreferencesWidgetPrivate(this))
{
    decorateEditors(TextEditorSettings::fontSettings());
    connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
            this, &CppCodeStylePreferencesWidget::decorateEditors);

    setVisualizeWhitespace(true);

//    m_ui->categoryTab->setCurrentIndex(0);
}

CppCodeStylePreferencesWidget::~CppCodeStylePreferencesWidget()
{
    delete d;
}

void CppCodeStylePreferencesWidget::setCodeStyle(CppCodeStylePreferences *codeStylePreferences)
{
    // code preferences
    m_preferences = codeStylePreferences;

    connect(m_preferences, &CppCodeStylePreferences::currentTabSettingsChanged,
            this, &CppCodeStylePreferencesWidget::setTabSettings);
    connect(m_preferences, &CppCodeStylePreferences::currentCodeStyleSettingsChanged,
            this, [this](const CppCodeStyleSettings &codeStyleSettings) {
        setCodeStyleSettings(codeStyleSettings);
    });

    connect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
            this, [this](TextEditor::ICodeStylePreferences *currentPreferences) {
        slotCurrentPreferencesChanged(currentPreferences);
    });

    setTabSettings(m_preferences->currentTabSettings());
    setCodeStyleSettings(m_preferences->currentCodeStyleSettings(), false);
    slotCurrentPreferencesChanged(m_preferences->currentPreferences(), false);

    updatePreview();
}

CppCodeStyleSettings CppCodeStylePreferencesWidget::cppCodeStyleSettings() const
{
    CppCodeStyleSettings set;

    set.indentBlockBraces = d->m_indentBlockBraces->isChecked();
    set.indentBlockBody = d->m_indentBlockBody->isChecked();
    set.indentClassBraces = d->m_indentClassBraces->isChecked();
    set.indentEnumBraces = d->m_indentEnumBraces->isChecked();
    set.indentNamespaceBraces = d->m_indentNamespaceBraces->isChecked();
    set.indentNamespaceBody = d->m_indentNamespaceBody->isChecked();
    set.indentAccessSpecifiers = d->m_indentAccessSpecifiers->isChecked();
    set.indentDeclarationsRelativeToAccessSpecifiers = d->m_indentDeclarationsRelativeToAccessSpecifiers->isChecked();
    set.indentFunctionBody = d->m_indentFunctionBody->isChecked();
    set.indentFunctionBraces = d->m_indentFunctionBraces->isChecked();
    set.indentSwitchLabels = d->m_indentSwitchLabels->isChecked();
    set.indentStatementsRelativeToSwitchLabels = d->m_indentCaseStatements->isChecked();
    set.indentBlocksRelativeToSwitchLabels = d->m_indentCaseBlocks->isChecked();
    set.indentControlFlowRelativeToSwitchLabels = d->m_indentCaseBreak->isChecked();
    set.bindStarToIdentifier = d->m_bindStarToIdentifier->isChecked();
    set.bindStarToTypeName = d->m_bindStarToTypeName->isChecked();
    set.bindStarToLeftSpecifier = d->m_bindStarToLeftSpecifier->isChecked();
    set.bindStarToRightSpecifier = d->m_bindStarToRightSpecifier->isChecked();
    set.extraPaddingForConditionsIfConfusingAlign = d->m_extraPaddingConditions->isChecked();
    set.alignAssignments = d->m_alignAssignments->isChecked();

    return set;
}

void CppCodeStylePreferencesWidget::setTabSettings(const TabSettings &settings)
{
    d->m_tabSettingsWidget->setTabSettings(settings);
}

TextEditor::TabSettings CppCodeStylePreferencesWidget::tabSettings() const
{
    return d->m_tabSettingsWidget->tabSettings();
}

void CppCodeStylePreferencesWidget::setCodeStyleSettings(const CppCodeStyleSettings &s, bool preview)
{
    const bool wasBlocked = m_blockUpdates;
    m_blockUpdates = true;
    d->m_indentBlockBraces->setChecked(s.indentBlockBraces);
    d->m_indentBlockBody->setChecked(s.indentBlockBody);
    d->m_indentClassBraces->setChecked(s.indentClassBraces);
    d->m_indentEnumBraces->setChecked(s.indentEnumBraces);
    d->m_indentNamespaceBraces->setChecked(s.indentNamespaceBraces);
    d->m_indentNamespaceBody->setChecked(s.indentNamespaceBody);
    d->m_indentAccessSpecifiers->setChecked(s.indentAccessSpecifiers);
    d->m_indentDeclarationsRelativeToAccessSpecifiers->setChecked(s.indentDeclarationsRelativeToAccessSpecifiers);
    d->m_indentFunctionBody->setChecked(s.indentFunctionBody);
    d->m_indentFunctionBraces->setChecked(s.indentFunctionBraces);
    d->m_indentSwitchLabels->setChecked(s.indentSwitchLabels);
    d->m_indentCaseStatements->setChecked(s.indentStatementsRelativeToSwitchLabels);
    d->m_indentCaseBlocks->setChecked(s.indentBlocksRelativeToSwitchLabels);
    d->m_indentCaseBreak->setChecked(s.indentControlFlowRelativeToSwitchLabels);
    d->m_bindStarToIdentifier->setChecked(s.bindStarToIdentifier);
    d->m_bindStarToTypeName->setChecked(s.bindStarToTypeName);
    d->m_bindStarToLeftSpecifier->setChecked(s.bindStarToLeftSpecifier);
    d->m_bindStarToRightSpecifier->setChecked(s.bindStarToRightSpecifier);
    d->m_extraPaddingConditions->setChecked(s.extraPaddingForConditionsIfConfusingAlign);
    d->m_alignAssignments->setChecked(s.alignAssignments);
    m_blockUpdates = wasBlocked;
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCurrentPreferencesChanged(ICodeStylePreferences *preferences, bool preview)
{
    const bool enable = !preferences->isReadOnly();
    for (QWidget *widget : d->m_controllers)
        widget->setEnabled(enable);

    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged()
{
    if (m_blockUpdates)
        return;

    emit codeStyleSettingsChanged(cppCodeStyleSettings());
    updatePreview();
}

void CppCodeStylePreferencesWidget::slotTabSettingsChanged(const TabSettings &settings)
{
    if (m_blockUpdates)
        return;

    emit tabSettingsChanged(settings);
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
    for (SnippetEditorWidget *preview : std::as_const(d->m_previews)) {
        preview->textDocument()->setTabSettings(ts);
        preview->setCodeStyle(cppCodeStylePreferences);

        QTextDocument *doc = preview->document();
        formatter.invalidateCache(doc);

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->textDocument()->indenter()->indentBlock(block, QChar::Null, ts);

            block = block.next();
        }
        applyRefactorings(doc, preview, ccss);
        tc.endEditBlock();
    }
}

void CppCodeStylePreferencesWidget::decorateEditors(const FontSettings &fontSettings)
{
    for (SnippetEditorWidget *editor : std::as_const(d->m_previews)) {
        editor->textDocument()->setFontSettings(fontSettings);
        SnippetProvider::decorateEditor(editor, CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
    }
}

void CppCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    for (SnippetEditorWidget *editor : std::as_const(d->m_previews)) {
        DisplaySettings displaySettings = editor->displaySettings();
        displaySettings.m_visualizeWhitespace = on;
        editor->setDisplaySettings(displaySettings);
    }
}

void CppCodeStylePreferencesWidget::addTab(CppCodeStyleWidget *page, QString tabName)
{
    if (!page)
        return;

    d->m_categoryTab->insertTab(0, page, tabName);
    d->m_categoryTab->setCurrentIndex(0);

    connect(page, &CppEditor::CppCodeStyleWidget::codeStyleSettingsChanged,
            this, [this](const CppEditor::CppCodeStyleSettings &settings) {
                setCodeStyleSettings(settings, true);
            });

    connect(page, &CppEditor::CppCodeStyleWidget::tabSettingsChanged,
            this, &CppCodeStylePreferencesWidget::setTabSettings);

    connect(this, &CppCodeStylePreferencesWidget::codeStyleSettingsChanged,
            page, &CppCodeStyleWidget::setCodeStyleSettings);

    connect(this, &CppCodeStylePreferencesWidget::tabSettingsChanged,
            page, &CppCodeStyleWidget::setTabSettings);

    connect(this, &CppCodeStylePreferencesWidget::applyEmitted,
            page, &CppCodeStyleWidget::apply);

    connect(this, &CppCodeStylePreferencesWidget::finishEmitted,
            page, &CppCodeStyleWidget::finish);

    page->synchronize();
}

void CppCodeStylePreferencesWidget::apply()
{
    if (m_preferences) {
        auto current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current) {
            current->setTabSettings(tabSettings());
            current->setCodeStyleSettings(cppCodeStyleSettings());
        }
    }

    emit applyEmitted();
}

void CppCodeStylePreferencesWidget::finish()
{
    emit finishEmitted();
}

// ------------------ CppCodeStyleSettingsPage

CppCodeStyleSettingsPage::CppCodeStyleSettingsPage()
{
    setId(Constants::CPP_CODE_STYLE_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppEditor", Constants::CPP_CODE_STYLE_SETTINGS_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
}

QWidget *CppCodeStyleSettingsPage::widget()
{
    if (!m_widget) {
        CppCodeStylePreferences *originalCodeStylePreferences = CppToolsSettings::instance()
                                                                    ->cppCodeStyle();
        m_pageCppCodeStylePreferences = new CppCodeStylePreferences();
        m_pageCppCodeStylePreferences->setDelegatingPool(
            originalCodeStylePreferences->delegatingPool());
        m_pageCppCodeStylePreferences->setCodeStyleSettings(
            originalCodeStylePreferences->codeStyleSettings());
        m_pageCppCodeStylePreferences->setCurrentDelegate(
            originalCodeStylePreferences->currentDelegate());
        // we set id so that it won't be possible to set delegate to the original prefs
        m_pageCppCodeStylePreferences->setId(originalCodeStylePreferences->id());
        m_widget = TextEditorSettings::codeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID)
                       ->createCodeStyleEditor(m_pageCppCodeStylePreferences);
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
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppEditor::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->tabSettings() != m_pageCppCodeStylePreferences->tabSettings()) {
            originalCppCodeStylePreferences->setTabSettings(m_pageCppCodeStylePreferences->tabSettings());
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppEditor::Constants::CPP_SETTINGS_ID), s);
        }
        if (originalCppCodeStylePreferences->currentDelegate() != m_pageCppCodeStylePreferences->currentDelegate()) {
            originalCppCodeStylePreferences->setCurrentDelegate(m_pageCppCodeStylePreferences->currentDelegate());
            originalCppCodeStylePreferences->toSettings(QLatin1String(CppEditor::Constants::CPP_SETTINGS_ID), s);
        }

        m_widget->apply();
    }
}

void CppCodeStyleSettingsPage::finish()
{
    if (m_widget)
        m_widget->finish();
    delete m_widget;
}

} // namespace CppEditor::Internal
