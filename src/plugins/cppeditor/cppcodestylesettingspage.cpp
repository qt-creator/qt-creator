// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodestylesettingspage.h"

#include "cppcodeformatter.h"
#include "cppcodestylepreferences.h"
#include "cppcodestylesnippets.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpppointerdeclarationformatter.h"
#include "cpptoolssettings.h"

#include <cppeditor/cppeditorconstants.h>

#include <cplusplus/Overview.h>
#include <cplusplus/pp.h>

#include <extensionsystem/pluginmanager.h>

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

#include <QCheckBox>
#include <QTabWidget>
#include <QTextBlock>
#include <QVBoxLayout>

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
    change.apply(textDocument);
}

// ------------------ CppCodeStyleSettingsWidget

class CppCodeStylePreferencesWidgetPrivate
{
public:
    CppCodeStylePreferencesWidgetPrivate(CppCodeStylePreferencesWidget *widget)
        : q(widget)
        , m_indentAccessSpecifiers(createCheckBox(Tr::tr("\"public\", \"protected\" and\n"
                                                         "\"private\" within class body")))
        , m_indentDeclarationsRelativeToAccessSpecifiers(
              createCheckBox(Tr::tr("Declarations relative to \"public\",\n"
                                "\"protected\" and \"private\"")))
        , m_indentFunctionBody(createCheckBox(Tr::tr("Statements within function body")))
        , m_indentBlockBody(createCheckBox(Tr::tr("Statements within blocks")))
        , m_indentNamespaceBody(createCheckBox(Tr::tr("Declarations within\n"
                                                  "\"namespace\" definition")))
        , m_indentClassBraces(createCheckBox(Tr::tr("Class declarations")))
        , m_indentNamespaceBraces(createCheckBox(Tr::tr("Namespace declarations")))
        , m_indentEnumBraces(createCheckBox(Tr::tr("Enum declarations")))
        , m_indentFunctionBraces(createCheckBox(Tr::tr("Function declarations")))
        , m_indentBlockBraces(createCheckBox(Tr::tr("Blocks")))
        , m_indentSwitchLabels(createCheckBox(Tr::tr("\"case\" or \"default\"")))
        , m_indentCaseStatements(createCheckBox(Tr::tr("Statements relative to\n"
                                                   "\"case\" or \"default\"")))
        , m_indentCaseBlocks(createCheckBox(Tr::tr("Blocks relative to\n"
                                               "\"case\" or \"default\"")))
        , m_indentCaseBreak(createCheckBox(Tr::tr("\"break\" statement relative to\n"
                                              "\"case\" or \"default\"")))
        , m_alignAssignments(createCheckBox(
            Tr::tr("Align after assignments"),
            Tr::tr("<html><head/><body>\n"
                   "Enables alignment to tokens after =, += etc. When the option is disabled, "
                     "regular continuation line indentation will be used.<br>\n"
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
        , m_extraPaddingConditions(createCheckBox(
            Tr::tr("Add extra padding to conditions\n"
                   "if they would align to the next line"),
            Tr::tr("<html><head/><body>\n"
                   "Adds an extra level of indentation to multiline conditions in the switch, "
                     "if, while and foreach statements if they would otherwise have the same or "
                     "less indentation than a nested statement.\n"
                   "\n"
                   "For four-spaces indentation only if statement conditions are affected. "
                     "Without extra padding:\n"
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
        , m_bindStarToIdentifier(createCheckBox(
            Tr::tr("Identifier"),
            Tr::tr("<html><head/><body>This does not apply to the star and reference symbol "
                     "in pointer/reference to functions and arrays, e.g.:\n"
               "<pre>   int (&rf)() = ...;\n"
               "   int (*pf)() = ...;\n"
               "\n"
               "   int (&ra)[2] = ...;\n"
               "   int (*pa)[2] = ...;\n"
               "\n"
               "</pre></body></html>")))
        , m_bindStarToTypeName(createCheckBox(Tr::tr("Type name")))
        , m_bindStarToLeftSpecifier(createCheckBox(Tr::tr("Left const/volatile")))
        , m_bindStarToRightSpecifier(createCheckBox(Tr::tr("Right const/volatile"),
                                                    Tr::tr("This does not apply to references.")))
        , m_tabSettingsWidget(new TabSettingsWidget)
    {
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_tabSettingsWidget->sizePolicy().hasHeightForWidth());
        m_tabSettingsWidget->setSizePolicy(sizePolicy);
        m_tabSettingsWidget->setFocusPolicy(Qt::TabFocus);
        QObject::connect(m_tabSettingsWidget, &TabSettingsWidget::settingsChanged,
                         q, &CppCodeStylePreferencesWidget::slotTabSettingsChanged);

        using namespace Layouting;

        QWidget *contentGroupWidget = nullptr;
        QWidget *bracesGroupWidget = nullptr;
        QWidget *switchGroupWidget = nullptr;
        QWidget *alignmentGroupWidget = nullptr;
        QWidget *typesGroupWidget = nullptr;

        const Group contentGroup {
            title(Tr::tr("Indent")),
            bindTo(&contentGroupWidget),
            Column {
                m_indentAccessSpecifiers,
                m_indentDeclarationsRelativeToAccessSpecifiers,
                m_indentFunctionBody,
                m_indentBlockBody,
                m_indentNamespaceBody,
                st
            }
        };

        const Group bracesGroup {
            title(Tr::tr("Indent Braces")),
            bindTo(&bracesGroupWidget),
            Column {
                m_indentClassBraces,
                m_indentNamespaceBraces,
                m_indentEnumBraces,
                m_indentFunctionBraces,
                m_indentBlockBraces,
                st
            }
        };

        const Group switchGroup {
            title(Tr::tr("Indent within \"switch\"")),
            bindTo(&switchGroupWidget),
            Column {
                m_indentSwitchLabels,
                m_indentCaseStatements,
                m_indentCaseBlocks,
                m_indentCaseBreak,
                st
            }
        };

        const Group alignmentGroup {
            title(Tr::tr("Align")),
            bindTo(&alignmentGroupWidget),
            Column {
                m_alignAssignments,
                m_extraPaddingConditions,
                st
            }
        };

        const Group typesGroup {
            title(Tr::tr("Bind '*' and '&&' in types/declarations to")),
            bindTo(&typesGroupWidget),
            Column {
                m_bindStarToIdentifier,
                m_bindStarToTypeName,
                m_bindStarToLeftSpecifier,
                m_bindStarToRightSpecifier,
                st
            }
        };

        Row {
            TabWidget {
                bindTo(&m_categoryTab),
                Tab { Tr::tr("General"),
                    Row { Column { m_tabSettingsWidget, st }, createPreview(0) }
                },
                Tab { Tr::tr("Content"), Row { contentGroup, createPreview(1) } },
                Tab { Tr::tr("Braces"), Row { bracesGroup, createPreview(2) } },
                Tab { Tr::tr("\"switch\""), Row { switchGroup, createPreview(3) } },
                Tab { Tr::tr("Alignment"), Row { alignmentGroup, createPreview(4) } },
                Tab { Tr::tr("Pointers and References"), Row { typesGroup, createPreview(5) } }
            }
        }.attachTo(q);

        m_categoryTab->setProperty("_q_custom_style_disabled", true);

        m_controllers.append(m_tabSettingsWidget);
        m_controllers.append(contentGroupWidget);
        m_controllers.append(bracesGroupWidget);
        m_controllers.append(switchGroupWidget);
        m_controllers.append(alignmentGroupWidget);
        m_controllers.append(typesGroupWidget);
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

    m_originalCppCodeStyleSettings = cppCodeStyleSettings();
    m_originalTabSettings = tabSettings();

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
    const bool enable = !preferences->isReadOnly() && (!preferences->isTemporarilyReadOnly()
                                                       || preferences->isAdditionalTabDisabled());
    for (QWidget *widget : d->m_controllers)
        widget->setEnabled(enable);

    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged()
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        auto current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setCodeStyleSettings(cppCodeStyleSettings());
    }

    emit codeStyleSettingsChanged(cppCodeStyleSettings());
    updatePreview();
}

void CppCodeStylePreferencesWidget::slotTabSettingsChanged(const TabSettings &settings)
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        auto current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setTabSettings(settings);
    }

    emit tabSettingsChanged(settings);
    updatePreview();
}

void CppCodeStylePreferencesWidget::updatePreview()
{
    CppCodeStylePreferences *cppCodeStylePreferences = m_preferences
            ? m_preferences
            : CppToolsSettings::cppCodeStyle();
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
    m_originalTabSettings = tabSettings();
    m_originalCppCodeStyleSettings = cppCodeStyleSettings();

    emit applyEmitted();
}

void CppCodeStylePreferencesWidget::finish()
{
    if (m_preferences) {
        auto current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentDelegate());
        if (current) {
            current->setCodeStyleSettings(m_originalCppCodeStyleSettings);
            current->setTabSettings(m_originalTabSettings);
        }
    }
    emit finishEmitted();
}

// CppCodeStyleSettingsPageWidget

class CppCodeStyleSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    CppCodeStyleSettingsPageWidget()
    {
        CppCodeStylePreferences *originalCodeStylePreferences = CppToolsSettings::cppCodeStyle();
        m_pageCppCodeStylePreferences = new CppCodeStylePreferences();
        m_pageCppCodeStylePreferences->setDelegatingPool(
            originalCodeStylePreferences->delegatingPool());
        m_pageCppCodeStylePreferences->setCodeStyleSettings(
            originalCodeStylePreferences->codeStyleSettings());
        m_pageCppCodeStylePreferences->setCurrentDelegate(
            originalCodeStylePreferences->currentDelegate());
        // we set id so that it won't be possible to set delegate to the original prefs
        m_pageCppCodeStylePreferences->setId(originalCodeStylePreferences->id());

        m_codeStyleEditor = TextEditorSettings::codeStyleFactory(CppEditor::Constants::CPP_SETTINGS_ID)
                                ->createCodeStyleEditor(m_pageCppCodeStylePreferences);

        auto hbox = new QVBoxLayout(this);
        hbox->addWidget(m_codeStyleEditor);
    }

    void apply() final
    {
        CppCodeStylePreferences *originalCppCodeStylePreferences = CppToolsSettings::cppCodeStyle();
        if (originalCppCodeStylePreferences->codeStyleSettings() != m_pageCppCodeStylePreferences->codeStyleSettings()) {
            originalCppCodeStylePreferences->setCodeStyleSettings(m_pageCppCodeStylePreferences->codeStyleSettings());
            originalCppCodeStylePreferences->toSettings(CppEditor::Constants::CPP_SETTINGS_ID);
        }
        if (originalCppCodeStylePreferences->tabSettings() != m_pageCppCodeStylePreferences->tabSettings()) {
            originalCppCodeStylePreferences->setTabSettings(m_pageCppCodeStylePreferences->tabSettings());
            originalCppCodeStylePreferences->toSettings(CppEditor::Constants::CPP_SETTINGS_ID);
        }
        if (originalCppCodeStylePreferences->currentDelegate() != m_pageCppCodeStylePreferences->currentDelegate()) {
            originalCppCodeStylePreferences->setCurrentDelegate(m_pageCppCodeStylePreferences->currentDelegate());
            originalCppCodeStylePreferences->toSettings(CppEditor::Constants::CPP_SETTINGS_ID);
        }

        m_codeStyleEditor->apply();
    }

    void finish() final
    {
        m_codeStyleEditor->finish();
    }

    CppCodeStylePreferences *m_pageCppCodeStylePreferences = nullptr;
    CodeStyleEditorWidget *m_codeStyleEditor;
};

// CppCodeStyleSettingsPage

CppCodeStyleSettingsPage::CppCodeStyleSettingsPage()
{
    setId(Constants::CPP_CODE_STYLE_SETTINGS_ID);
    setDisplayName(Tr::tr("Code Style"));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new CppCodeStyleSettingsPageWidget; });
}

} // namespace CppEditor::Internal
