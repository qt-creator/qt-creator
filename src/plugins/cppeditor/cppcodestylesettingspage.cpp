// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodestylesettingspage.h"

#include "cppcodeformatter.h"
#include "cppcodestylesnippets.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpppointerdeclarationformatter.h"
#include "cpptoolssettings.h"

#include <cppeditor/cppeditorconstants.h>

#include <coreplugin/dialogs/ioptionspage.h>

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
#include <texteditor/textdocument.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QTextBlock>
#include <QVBoxLayout>

using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

namespace Internal {

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
    cppDocument->parse(Document::ParseTranslationUnit);
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
    CppCodeStylePreferencesWidgetPrivate(CppCodeStylePreferencesWidget *widget,
                                         CppCodeStylePreferences *codeStylePreferences)
        : q(widget)
        , m_preferences(codeStylePreferences)
    {
        setupCheckBox(m_indentAccessSpecifiers,
                      Tr::tr("\"public\", \"protected\" and\n\"private\" within class body"));
        setupCheckBox(m_indentDeclarationsRelativeToAccessSpecifiers,
                      Tr::tr("Declarations relative to \"public\",\n"
                             "\"protected\" and \"private\""));
        setupCheckBox(m_indentFunctionBody, Tr::tr("Statements within function body"));
        setupCheckBox(m_indentBlockBody, Tr::tr("Statements within blocks"));
        setupCheckBox(m_indentNamespaceBody,
                      Tr::tr("Declarations within\n\"namespace\" definition"));
        setupCheckBox(m_indentClassBraces, Tr::tr("Class declarations"));
        setupCheckBox(m_indentNamespaceBraces, Tr::tr("Namespace declarations"));
        setupCheckBox(m_indentEnumBraces, Tr::tr("Enum declarations"));
        setupCheckBox(m_indentFunctionBraces, Tr::tr("Function declarations"));
        setupCheckBox(m_indentBlockBraces, Tr::tr("Blocks"));
        setupCheckBox(m_indentSwitchLabels, Tr::tr("\"case\" or \"default\""));
        setupCheckBox(m_indentCaseStatements,
                      Tr::tr("Statements relative to\n\"case\" or \"default\""));
        setupCheckBox(m_indentCaseBlocks, Tr::tr("Blocks relative to\n\"case\" or \"default\""));
        setupCheckBox(m_indentCaseBreak,
                      Tr::tr("\"break\" statement relative to\n\"case\" or \"default\""));
        setupCheckBox(m_alignAssignments,
                      Tr::tr("Align after assignments"),
                      Tr::tr("<html><head/><body>\n"
                             "Enables alignment to tokens after =, += etc. When the option is "
                             "disabled, regular continuation line indentation will be used.<br>\n"
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
                             "</body></html>"));
        setupCheckBox(m_extraPaddingConditions,
                      Tr::tr("Add extra padding to conditions\n"
                             "if they would align to the next line"),
                      Tr::tr("<html><head/><body>\n"
                             "Adds an extra level of indentation to multiline conditions in the "
                             "switch, if, while and foreach statements if they would otherwise "
                             "have the same or less indentation than a nested statement.\n"
                             "\n"
                             "For four-spaces indentation only if statement conditions are "
                             "affected. Without extra padding:\n"
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
                             "</body></html>"));
        setupCheckBox(m_bindStarToIdentifier,
                      Tr::tr("Identifier"),
                      Tr::tr("<html><head/><body>This does not apply to the star and reference "
                             "symbol in pointer/reference to functions and arrays, e.g.:\n"
                             "<pre>   int (&rf)() = ...;\n"
                             "   int (*pf)() = ...;\n"
                             "\n"
                             "   int (&ra)[2] = ...;\n"
                             "   int (*pa)[2] = ...;\n"
                             "\n"
                             "</pre></body></html>"));
        setupCheckBox(m_bindStarToTypeName, Tr::tr("Type name"));
        setupCheckBox(m_bindStarToLeftSpecifier, Tr::tr("Left const/volatile"));
        setupCheckBox(m_bindStarToRightSpecifier,
                      Tr::tr("Right const/volatile"),
                      Tr::tr("This does not apply to references."));

        QObject::connect(&m_tabSettingsWidget, &TabSettings::changed,
                         q, [this] { slotTabSettingsChanged(); });

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
                &m_indentAccessSpecifiers,
                &m_indentDeclarationsRelativeToAccessSpecifiers,
                &m_indentFunctionBody,
                &m_indentBlockBody,
                &m_indentNamespaceBody,
                st
            }
        };

        const Group bracesGroup {
            title(Tr::tr("Indent Braces")),
            bindTo(&bracesGroupWidget),
            Column {
                &m_indentClassBraces,
                &m_indentNamespaceBraces,
                &m_indentEnumBraces,
                &m_indentFunctionBraces,
                &m_indentBlockBraces,
                st
            }
        };

        const Group switchGroup {
            title(Tr::tr("Indent within \"switch\"")),
            bindTo(&switchGroupWidget),
            Column {
                &m_indentSwitchLabels,
                &m_indentCaseStatements,
                &m_indentCaseBlocks,
                &m_indentCaseBreak,
                st
            }
        };

        const Group alignmentGroup {
            title(Tr::tr("Align")),
            bindTo(&alignmentGroupWidget),
            Column {
                &m_alignAssignments,
                &m_extraPaddingConditions,
                st
            }
        };

        const Group typesGroup {
            title(Tr::tr("Bind '*' and '&&' in types/declarations to")),
            bindTo(&typesGroupWidget),
            Column {
                &m_bindStarToIdentifier,
                &m_bindStarToTypeName,
                &m_bindStarToLeftSpecifier,
                &m_bindStarToRightSpecifier,
                st
            }
        };

        QSizePolicy sizePolicy;
        sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
        m_statementMacros.setToolTip(
            Tr::tr("Macros that can be used as statements without a trailing semicolon."));
        m_statementMacros.setSizePolicy(sizePolicy);
        // clang-format off
        const Group statementMacrosGroup {
            title(Tr::tr("Statement Macros")),
            Column { &m_statementMacros }
        };
        // clang-format on
        QObject::connect(&m_statementMacros, &QPlainTextEdit::textChanged, q, [this] {
            m_handlingStatementMacroChange = true;
            slotCodeStyleSettingsChanged();
            m_handlingStatementMacroChange = false;
        });

        m_generalSettingsRow = Column { m_tabSettingsWidget, statementMacrosGroup }.emerge();

        Row {
            TabWidget {
                bindTo(&m_categoryTab),
                Tab { Tr::tr("General"),
                    Row { m_generalSettingsRow, createPreview(0) }
                },
                Tab { Tr::tr("Content"), Row { contentGroup, createPreview(1) } },
                Tab { Tr::tr("Braces"), Row { bracesGroup, createPreview(2) } },
                Tab { Tr::tr("\"switch\""), Row { switchGroup, createPreview(3) } },
                Tab { Tr::tr("Alignment"), Row { alignmentGroup, createPreview(4) } },
                Tab { Tr::tr("Pointers and References"), Row { typesGroup, createPreview(5) } }
            }
        }.attachTo(q);

        m_categoryTab->setProperty("_q_custom_style_disabled", true);

        m_controllers.append(contentGroupWidget);
        m_controllers.append(bracesGroupWidget);
        m_controllers.append(switchGroupWidget);
        m_controllers.append(alignmentGroupWidget);
        m_controllers.append(typesGroupWidget);

        decorateEditors(globalFontSettings().data());
        QObject::connect(&globalFontSettings(), &FontSettings::changed,
                         q, [this] { decorateEditors(globalFontSettings().data()); });

        setVisualizeWhitespace(true);

        QObject::connect(m_preferences, &CppCodeStylePreferences::currentTabSettingsChanged,
                         q, [this](const TabSettingsData &s) { setTabSettings(s); });
        QObject::connect(m_preferences, &CppCodeStylePreferences::currentValueChanged, q, [this] {
            setCodeStyleSettings(m_preferences->currentCodeStyleSettings());
        });
        QObject::connect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
                         q, [this](TextEditor::ICodeStylePreferences *currentPreferences) {
            slotCurrentPreferencesChanged(currentPreferences);
        });

        setTabSettings(m_preferences->currentTabSettings());
        setCodeStyleSettings(m_preferences->currentCodeStyleSettings(), false);
        slotCurrentPreferencesChanged(m_preferences->currentPreferences(), false);

        m_originalCppCodeStyleSettings = cppCodeStyleSettings();
        m_originalTabSettings = tabSettings();

        updatePreview();
    }

    void setupCheckBox(QCheckBox &checkBox, const QString &text, const QString &toolTip = {})
    {
        checkBox.setText(text);
        checkBox.setToolTip(toolTip);
        QObject::connect(&checkBox, &QCheckBox::toggled,
                         q, [this] { slotCodeStyleSettingsChanged(); });
    }

    SnippetEditorWidget *createPreview(int i)
    {
        SnippetEditorWidget *editor = new SnippetEditorWidget;
        editor->setPlainText(QLatin1String(Constants::DEFAULT_CODE_STYLE_SNIPPETS[i]));
        m_previews.append(editor);
        return editor;
    }

    CppCodeStyleSettings cppCodeStyleSettings() const;
    void setTabSettings(const TabSettingsData &settings);
    TabSettingsData tabSettings() const;
    void setCodeStyleSettings(const CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(ICodeStylePreferences *preferences, bool preview = true);
    void slotCodeStyleSettingsChanged();
    void slotTabSettingsChanged();
    void updatePreview();
    void decorateEditors(const FontSettingsData &fontSettings);
    void setVisualizeWhitespace(bool on);
    void apply();
    void cancel();

    CppCodeStylePreferencesWidget *q = nullptr;

    QCheckBox m_indentAccessSpecifiers;
    QCheckBox m_indentDeclarationsRelativeToAccessSpecifiers;
    QCheckBox m_indentFunctionBody;
    QCheckBox m_indentBlockBody;
    QCheckBox m_indentNamespaceBody;
    QCheckBox m_indentClassBraces;
    QCheckBox m_indentNamespaceBraces;
    QCheckBox m_indentEnumBraces;
    QCheckBox m_indentFunctionBraces;
    QCheckBox m_indentBlockBraces;
    QCheckBox m_indentSwitchLabels;
    QCheckBox m_indentCaseStatements;
    QCheckBox m_indentCaseBlocks;
    QCheckBox m_indentCaseBreak;
    QCheckBox m_alignAssignments;
    QCheckBox m_extraPaddingConditions;
    QCheckBox m_bindStarToIdentifier;
    QCheckBox m_bindStarToTypeName;
    QCheckBox m_bindStarToLeftSpecifier;
    QCheckBox m_bindStarToRightSpecifier;

    QPlainTextEdit m_statementMacros;
    QList<SnippetEditorWidget *> m_previews;
    QList<QWidget *> m_controllers;

    QTabWidget *m_categoryTab = nullptr;
    QWidget *m_generalSettingsRow = nullptr;
    TabSettings m_tabSettingsWidget;
    bool m_handlingStatementMacroChange = false;

    CppCodeStylePreferences *m_preferences = nullptr;
    CppCodeStyleSettings m_originalCppCodeStyleSettings;
    TabSettingsData m_originalTabSettings;
    bool m_blockUpdates = false;
};

CppCodeStyleSettings CppCodeStylePreferencesWidgetPrivate::cppCodeStyleSettings() const
{
    CppCodeStyleSettings set;

    set.statementMacros
        = Utils::transform(m_statementMacros.toPlainText().trimmed().split('\n',
                                                                           Qt::SkipEmptyParts),
                           [](const QString &line) { return line.trimmed(); });
    set.indentBlockBraces = m_indentBlockBraces.isChecked();
    set.indentBlockBody = m_indentBlockBody.isChecked();
    set.indentClassBraces = m_indentClassBraces.isChecked();
    set.indentEnumBraces = m_indentEnumBraces.isChecked();
    set.indentNamespaceBraces = m_indentNamespaceBraces.isChecked();
    set.indentNamespaceBody = m_indentNamespaceBody.isChecked();
    set.indentAccessSpecifiers = m_indentAccessSpecifiers.isChecked();
    set.indentDeclarationsRelativeToAccessSpecifiers
        = m_indentDeclarationsRelativeToAccessSpecifiers.isChecked();
    set.indentFunctionBody = m_indentFunctionBody.isChecked();
    set.indentFunctionBraces = m_indentFunctionBraces.isChecked();
    set.indentSwitchLabels = m_indentSwitchLabels.isChecked();
    set.indentStatementsRelativeToSwitchLabels = m_indentCaseStatements.isChecked();
    set.indentBlocksRelativeToSwitchLabels = m_indentCaseBlocks.isChecked();
    set.indentControlFlowRelativeToSwitchLabels = m_indentCaseBreak.isChecked();
    set.bindStarToIdentifier = m_bindStarToIdentifier.isChecked();
    set.bindStarToTypeName = m_bindStarToTypeName.isChecked();
    set.bindStarToLeftSpecifier = m_bindStarToLeftSpecifier.isChecked();
    set.bindStarToRightSpecifier = m_bindStarToRightSpecifier.isChecked();
    set.extraPaddingForConditionsIfConfusingAlign = m_extraPaddingConditions.isChecked();
    set.alignAssignments = m_alignAssignments.isChecked();

    return set;
}

void CppCodeStylePreferencesWidgetPrivate::setTabSettings(const TabSettingsData &settings)
{
    m_tabSettingsWidget.setData(settings);
}

TextEditor::TabSettingsData CppCodeStylePreferencesWidgetPrivate::tabSettings() const
{
    return m_tabSettingsWidget.data();
}

void CppCodeStylePreferencesWidgetPrivate::setCodeStyleSettings(const CppCodeStyleSettings &s,
                                                                bool preview)
{
    const bool wasBlocked = m_blockUpdates;
    m_blockUpdates = true;
    if (!m_handlingStatementMacroChange)
        m_statementMacros.setPlainText(s.statementMacros.join('\n'));
    m_indentBlockBraces.setChecked(s.indentBlockBraces);
    m_indentBlockBody.setChecked(s.indentBlockBody);
    m_indentClassBraces.setChecked(s.indentClassBraces);
    m_indentEnumBraces.setChecked(s.indentEnumBraces);
    m_indentNamespaceBraces.setChecked(s.indentNamespaceBraces);
    m_indentNamespaceBody.setChecked(s.indentNamespaceBody);
    m_indentAccessSpecifiers.setChecked(s.indentAccessSpecifiers);
    m_indentDeclarationsRelativeToAccessSpecifiers.setChecked(
        s.indentDeclarationsRelativeToAccessSpecifiers);
    m_indentFunctionBody.setChecked(s.indentFunctionBody);
    m_indentFunctionBraces.setChecked(s.indentFunctionBraces);
    m_indentSwitchLabels.setChecked(s.indentSwitchLabels);
    m_indentCaseStatements.setChecked(s.indentStatementsRelativeToSwitchLabels);
    m_indentCaseBlocks.setChecked(s.indentBlocksRelativeToSwitchLabels);
    m_indentCaseBreak.setChecked(s.indentControlFlowRelativeToSwitchLabels);
    m_bindStarToIdentifier.setChecked(s.bindStarToIdentifier);
    m_bindStarToTypeName.setChecked(s.bindStarToTypeName);
    m_bindStarToLeftSpecifier.setChecked(s.bindStarToLeftSpecifier);
    m_bindStarToRightSpecifier.setChecked(s.bindStarToRightSpecifier);
    m_extraPaddingConditions.setChecked(s.extraPaddingForConditionsIfConfusingAlign);
    m_alignAssignments.setChecked(s.alignAssignments);
    m_blockUpdates = wasBlocked;
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidgetPrivate::slotCurrentPreferencesChanged(
    ICodeStylePreferences *preferences, bool preview)
{
    const bool enable = !preferences->isReadOnly();
    for (QWidget *widget : std::as_const(m_controllers))
        widget->setEnabled(enable);
    m_generalSettingsRow->setEnabled(enable);

    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidgetPrivate::slotCodeStyleSettingsChanged()
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        auto current = dynamic_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setCodeStyleSettings(cppCodeStyleSettings());
    }

    updatePreview();
}

void CppCodeStylePreferencesWidgetPrivate::slotTabSettingsChanged()
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        auto current = dynamic_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setTabSettings(m_tabSettingsWidget.data());
    }

    updatePreview();
}

void CppCodeStylePreferencesWidgetPrivate::updatePreview()
{
    CppCodeStylePreferences *cppCodeStylePreferences
            = m_preferences ? m_preferences : cppCodeStyle();

    const CppCodeStyleSettings ccss = cppCodeStylePreferences->currentCodeStyleSettings();
    const TabSettingsData ts = cppCodeStylePreferences->currentTabSettings();
    QtStyleCodeFormatter formatter(ts, ccss);
    for (SnippetEditorWidget *preview : std::as_const(m_previews)) {
        preview->textDocument()->setTabSettings(ts);
        preview->textDocument()->setCodeStyle(cppCodeStylePreferences);

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

void CppCodeStylePreferencesWidgetPrivate::decorateEditors(const FontSettingsData &fontSettings)
{
    for (SnippetEditorWidget *editor : std::as_const(m_previews)) {
        editor->textDocument()->setFontSettings(fontSettings);
        SnippetProvider::decorateEditor(editor, CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
    }
}

void CppCodeStylePreferencesWidgetPrivate::setVisualizeWhitespace(bool on)
{
    for (SnippetEditorWidget *editor : std::as_const(m_previews)) {
        DisplaySettingsData displaySettings = editor->displaySettings();
        displaySettings.m_visualizeWhitespace = on;
        editor->setDisplaySettings(displaySettings);
    }
}

void CppCodeStylePreferencesWidgetPrivate::apply()
{
    m_originalTabSettings = tabSettings();
    m_originalCppCodeStyleSettings = cppCodeStyleSettings();
}

void CppCodeStylePreferencesWidgetPrivate::cancel()
{
    if (m_preferences) {
        auto current = dynamic_cast<CppCodeStylePreferences *>(m_preferences->currentDelegate());
        if (current) {
            current->setCodeStyleSettings(m_originalCppCodeStyleSettings);
            current->setTabSettings(m_originalTabSettings);
        }
    }
}

} // namespace Internal

CppCodeStylePreferencesWidget::CppCodeStylePreferencesWidget(CppCodeStylePreferences *codeStylePreferences)
    : d{new Internal::CppCodeStylePreferencesWidgetPrivate(this, codeStylePreferences)}
{}

CppCodeStylePreferencesWidget::~CppCodeStylePreferencesWidget()
{
    delete d;
}

void CppCodeStylePreferencesWidget::apply()
{
    d->apply();
}

void CppCodeStylePreferencesWidget::cancel()
{
    d->cancel();
}

namespace Internal {

// CppCodeStyleSettingsPage

class CppCodeStyleSettingsPage : public Core::IOptionsPage
{
public:
    CppCodeStyleSettingsPage()
    {
        setId(Constants::CPP_CODE_STYLE_SETTINGS_ID);
        setDisplayName(Tr::tr("Code Style"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setSettingsProvider([] {
            static CodeStyleAspect theSettings(cppCodeStyle(), CppEditor::Constants::CPP_SETTINGS_ID);
            return &theSettings;
        });
    }
};

void setupCppCodeStyleSettings()
{
    static CppCodeStyleSettingsPage theCppCodeStyleSettingsPage;
}

} // namespace Internal

} // namespace CppEditor
