// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glsleditor.h"

#include "glslautocompleter.h"
#include "glslcompletionassist.h"
#include "glsleditorconstants.h"
#include "glsleditortr.h"
#include "glslhighlighter.h"
#include "glslindenter.h"

#include <glsl/glslastdump.h>
#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslengine.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <cplusplus/SimpleLexer.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/textdocument.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/changeset.h>
#include <utils/icon.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>
#include <utils/uncommentselection.h>

#include <QCoreApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QTextBlock>
#include <QTimer>
#include <QTreeView>

using namespace TextEditor;
using namespace GLSL;

namespace GlslEditor::Internal {

static int versionFor(const QString &source)
{
    CPlusPlus::SimpleLexer lexer;
    lexer.setPreprocessorMode(false);
    const CPlusPlus::Tokens tokens = lexer(source);

    int version = -1;
//    QString profile;
    const int end = tokens.size();
    for (int it = 0; it + 2 < end; ++it) {
        const CPlusPlus::Token &token = tokens.at(it);
        if (token.isComment())
            continue;
        if (token.kind() == CPlusPlus::T_POUND) {
            const int line = token.lineno;
            const CPlusPlus::Token &successor = tokens.at(it + 1);
            if (line != successor.lineno)
                break;
            if (successor.kind() != CPlusPlus::T_IDENTIFIER)
                break;
            if (source.mid(successor.bytesBegin(), successor.bytes()) != "version")
                break;

            const CPlusPlus::Token &versionToken = tokens.at(it + 2);
            if (line != versionToken.lineno)
                break;
            if (versionToken.kind() != CPlusPlus::T_NUMERIC_LITERAL)
                break;
            version = source.mid(versionToken.bytesBegin(), versionToken.bytes()).toInt();

//            if (version >= 150 && it + 3 < end) {
//                const CPlusPlus::Token &profileToken = tokens.at(it + 3);
//                if (line != profileToken.lineno)
//                    break;
//                if (profileToken.kind() != CPlusPlus::T_IDENTIFIER)
//                    break;
//                profile = source.mid(profileToken.bytesBegin(), profileToken.bytes());
//            }
            break;
        }
        break;
    }
    return version;
}

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150
};

class InitFile final
{
public:
    explicit InitFile(const QString &fileName, bool cutVulkanRemoved = false)
        : m_fileName(fileName), m_cutVulkanRemoved(cutVulkanRemoved) {}

    ~InitFile() { delete m_engine; }

    GLSL::Engine *engine() const
    {
        if (!m_engine)
            initialize();
        return m_engine;
    }

    GLSL::TranslationUnitAST *ast() const
    {
        if (!m_ast)
            initialize();
        return m_ast;
    }

private:
    void initialize() const
    {
        // Parse the builtins for any language variant so we can use all keywords.
        const int variant = m_cutVulkanRemoved
                       ? GLSL::Lexer::Variant_All
                       : (GLSL::Lexer::Variant_All & ~GLSL::Lexer::Variant_Vulkan);

        QByteArray code;
        QFile file(Core::ICore::resourcePath("glsl").pathAppended(m_fileName).toUrlishString());
        if (file.open(QFile::ReadOnly))
            code = file.readAll();

        if (m_cutVulkanRemoved) {
            const int index = code.indexOf("//// Vulkan removed variables and functions");
            if (index != -1)
                code.truncate(index);
        }

        m_engine = new GLSL::Engine();
        GLSL::Parser parser(m_engine, code.constData(), code.size(), variant);
        m_ast = parser.parse();
        QTC_CHECK(m_ast);
    }

    QString m_fileName;
    mutable GLSL::Engine *m_engine = nullptr;
    mutable GLSL::TranslationUnitAST *m_ast = nullptr;
    bool m_cutVulkanRemoved = false;
};

static const InitFile *fragmentShaderInit(int variant)
{
    static InitFile glsl_es_100_frag{"glsl_es_100.frag"};
    static InitFile glsl_120_frag{"glsl_120.frag"};
    static InitFile glsl_330_frag{"glsl_330.frag"};
    static InitFile glsl_460_frag{"glsl_460.frag"};

    if (variant & GLSL::Lexer::Variant_GLSL_460)
        return &glsl_460_frag;

    if (variant & GLSL::Lexer::Variant_GLSL_400)
        return &glsl_330_frag;

    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return  &glsl_120_frag;

    return &glsl_es_100_frag;
}

static const InitFile *vertexShaderInit(int variant)
{
    static InitFile glsl_es_100_vert{"glsl_es_100.vert"};
    static InitFile glsl_120_vert{"glsl_120.vert"};
    static InitFile glsl_330_vert{"glsl_330.vert"};
    static InitFile glsl_330_vert_modified{"glsl_330.vert", true};
    static InitFile glsl_460_vert{"glsl_460.vert"};
    static InitFile glsl_460_vert_modified{"glsl_460.vert", true};

    if (variant & GLSL::Lexer::Variant_GLSL_460) {
        if (variant & GLSL::Lexer::Variant_Vulkan)
            return &glsl_460_vert_modified;
        return &glsl_460_vert;
    }

    if (variant & GLSL::Lexer::Variant_GLSL_400) {
        if (variant & GLSL::Lexer::Variant_Vulkan)
            return &glsl_330_vert_modified;
        return &glsl_330_vert;
    }

    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return &glsl_120_vert;

    return &glsl_es_100_vert;
}

static const InitFile *shaderInit(int variant)
{
    static InitFile glsl_es_100_common{"glsl_es_100_common.glsl"};
    static InitFile glsl_120_common{"glsl_120_common.glsl"};
    static InitFile glsl_330_common{"glsl_330_common.glsl"};
    static InitFile glsl_330_common_modified{"glsl_330_common.glsl", true};
    static InitFile glsl_460_common{"glsl_460_common.glsl"};
    static InitFile glsl_460_common_modified{"glsl_460_common.glsl", true};

    if (variant & GLSL::Lexer::Variant_GLSL_460) {
        if (variant & GLSL::Lexer::Variant_Vulkan)
            return &glsl_460_common_modified;
        return &glsl_460_common;
    }

    if (variant & GLSL::Lexer::Variant_GLSL_400) {
        if (variant & GLSL::Lexer::Variant_Vulkan)
            return &glsl_330_common_modified;
        return &glsl_330_common;
    }

    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return &glsl_120_common;

    return &glsl_es_100_common;
}

static const InitFile *vulkanInit(int /*variant*/)
{
    static InitFile glsl_vulkan{"glsl_vulkan.glsl"};
    return &glsl_vulkan;
}

class CreateRangesMarkSemanticDetails: protected Visitor
{
    QTextDocument *textDocument;
    Document::Ptr glslDocument;
    Namespace *globalNamespace;
    QList<QTextEdit::ExtraSelection> marked;
    QTextCharFormat functionCallFormat;
    QTextCharFormat memberFormat;
    QTextCharFormat globalVarFormat;
    QTextCharFormat layoutIdFormat;

public:
    CreateRangesMarkSemanticDetails(QTextDocument *textDocument, Document::Ptr glslDocument,
                                    Namespace *global)
        : textDocument(textDocument), glslDocument(glslDocument), globalNamespace(global)
    {
        const TextEditor::FontSettings &fontSettings
                = TextEditor::TextEditorSettings::fontSettings();
        functionCallFormat = fontSettings.toTextCharFormat(TextEditor::C_FUNCTION);
        memberFormat = fontSettings.toTextCharFormat(TextEditor::C_FIELD);
        globalVarFormat = fontSettings.toTextCharFormat(TextEditor::C_GLOBAL);
        layoutIdFormat = fontSettings.toTextCharFormat(TextEditor::C_ENUMERATION);
    }

    void operator()(AST *ast) { accept(ast); }

    const QList<QTextEdit::ExtraSelection> markedSemantics() const { return marked; }

protected:
    using GLSL::Visitor::visit;

    void endVisit(CompoundStatementAST *ast) override
    {
        if (ast->symbol) {
            QTC_ASSERT(ast->length != -1, return);
            QTextCursor tc(textDocument);
            tc.setPosition(ast->position);
            tc.setPosition(ast->position + ast->length, QTextCursor::KeepAnchor);
            glslDocument->addRange(tc, ast->symbol);
        }
    }

    void endVisit(FunctionCallExpressionAST *ast) override
    {
        if (auto id = ast->id) {
            if (auto name = id->name) {
                if (globalNamespace->find(*name))
                    createGlobalMemberEntry(id, functionCallFormat);
            }
        }
    }

    void endVisit(MemberAccessExpressionAST *ast) override
    {
        if (auto member = ast->field) {
            QTC_ASSERT(ast->length != -1, return);
            QTextCursor tc(textDocument);
            const int position = ast->position + ast->length + - member->length();
            tc.setPosition(position);
            tc.setPosition(position + member->length(), QTextCursor::KeepAnchor);
            QTextEdit::ExtraSelection sel;
            sel.cursor = tc;
            sel.format = memberFormat;
            marked.append(sel);
        }
    }

    void endVisit(IdentifierExpressionAST *ast) override
    {
        if (auto name = ast->name; name && name->startsWith("gl_")) {
            if (globalNamespace->find(*name))
                createGlobalMemberEntry(ast, globalVarFormat);
        }
    }

    void endVisit(StructTypeAST::Field *ast) override
    {
        if (auto name = ast->name; name && name->startsWith("gl_")) {
            if (globalNamespace->find(*name))
                createGlobalMemberEntry(ast, globalVarFormat);
        }
    }

    void endVisit(InterfaceBlockAST *ast) override
    {
        if (auto name = ast->name; name && name->startsWith("gl_")) {
            if (globalNamespace->find(*name))
                createGlobalMemberEntry(ast, globalVarFormat);
        }
    }

    void endVisit(LayoutQualifierAST *ast) override
    {
        if (ast->name) {
            QTC_ASSERT(ast->length != -1, return);
            QTextCursor tc(textDocument);
            tc.setPosition(ast->position);
            tc.setPosition(ast->position + ast->name->length(), QTextCursor::KeepAnchor);
            QTextEdit::ExtraSelection sel;
            sel.cursor = tc;
            sel.format = layoutIdFormat;
            marked.append(sel);
        }
    }

private:
    void createGlobalMemberEntry(AST *ast, const QTextCharFormat &format)
    {
        QTC_ASSERT(ast->length != -1, return);
        QTextCursor tc(textDocument);
        tc.setPosition(ast->position);
        tc.setPosition(ast->position + ast->length, QTextCursor::KeepAnchor);
        QTextEdit::ExtraSelection sel;
        sel.cursor = tc;
        sel.format = format;
        marked.append(sel);
    }
};

static Utils::Icon vulkanIcon({{":/glsleditor/images/vulkan.png", Utils::Theme::IconsBaseColor}});

//
//  GlslEditorWidget
//

class GlslEditorWidget : public TextEditorWidget
{
public:
    GlslEditorWidget();

    int editorRevision() const;
    bool isOutdated() const;

    QSet<QString> identifiers() const;

    std::unique_ptr<AssistInterface> createAssistInterface(AssistKind assistKind,
                                                           AssistReason reason) const override;

private:
    void updateDocumentNow();
    void setSelectedElements();
    void onTooltipRequested(const QPoint &point, int pos);
    QString wordUnderCursor() const;
    QTextCursor cursorForDiagnosticMessage(const DiagnosticMessage &message) const;

    QTimer m_updateDocumentTimer;
    QComboBox *m_outlineCombo = nullptr;
    QToolButton *m_vulkanSupport = nullptr;
    Document::Ptr m_glslDocument;
};

GlslEditorWidget::GlslEditorWidget()
{
    setAutoCompleter(new GlslCompleter);

    m_updateDocumentTimer.setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer.setSingleShot(true);
    connect(&m_updateDocumentTimer, &QTimer::timeout,
            this, &GlslEditorWidget::updateDocumentNow);

    connect(this, &PlainTextEdit::textChanged, [this] { m_updateDocumentTimer.start(); });

    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);

    // ### m_outlineCombo->setModel(m_outlineModel);

    auto treeView = new QTreeView;
    treeView->header()->hide();
    treeView->setItemsExpandable(false);
    treeView->setRootIsDecorated(false);
    m_outlineCombo->setView(treeView);
    treeView->expandAll();

    //m_outlineCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    m_vulkanSupport = new QToolButton;
    m_vulkanSupport->setCheckable(true);
    m_vulkanSupport->setChecked(true);
    m_vulkanSupport->setIcon(vulkanIcon.icon());
    const auto updateVulkanToolTip = [this] {
        m_vulkanSupport->setToolTip(
            m_vulkanSupport->isChecked() ? Tr::tr("Vulkan support is enabled.")
                                         : Tr::tr("Vulkan support is disabled."));
    };
    updateVulkanToolTip();

    insertExtraToolBarWidget(TextEditorWidget::Left, m_outlineCombo);
    insertExtraToolBarWidget(TextEditorWidget::Right, m_vulkanSupport);

    connect(m_vulkanSupport, &QToolButton::clicked, this, [this, updateVulkanToolTip] {
        updateVulkanToolTip();
        m_updateDocumentTimer.start();
    });
    connect(this, &TextEditorWidget::tooltipRequested, this, &GlslEditorWidget::onTooltipRequested);
}

int GlslEditorWidget::editorRevision() const
{
    //return document()->revision();
    return 0;
}

bool GlslEditorWidget::isOutdated() const
{
//    if (m_semanticInfo.revision() != editorRevision())
//        return true;

    return false;
}

QString GlslEditorWidget::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    const QChar ch = document()->characterAt(tc.position() - 1);
    // make sure that we're not at the start of the next word.
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word = tc.selectedText();
    return word;
}

QTextCursor GlslEditorWidget::cursorForDiagnosticMessage(const DiagnosticMessage &message) const
{
    const DiagnosticMessage::Location &location = message.location();
    if (location.length >= 0) {
        QTextCursor cursor(document());
        cursor.setPosition(location.position);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, location.length);
        return cursor;
    }
    QTC_CHECK(false);
    // we only have a line number
    QTextCursor cursor(document()->findBlockByNumber(message.line() - 1));
    static const QRegularExpression ws("^(\\s+)");
    const QRegularExpressionMatch match = ws.match(cursor.block().text());
    if (match.hasMatch())
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, match.capturedLength(1));
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    return cursor;
}

void GlslEditorWidget::updateDocumentNow()
{
    m_updateDocumentTimer.stop();

    int variant = languageVariant(textDocument()->mimeType());
    const QString contents = toPlainText(); // get the code from the editor
    int version = versionFor(contents);
    if (version >= 330) {
        if (version >= 420)
            variant |= GLSL::Lexer::Variant_GLSL_460;
        else
            variant |= GLSL::Lexer::Variant_GLSL_400;
        if (m_vulkanSupport->isChecked())
            variant |= GLSL::Lexer::Variant_Vulkan;
    }

    const QByteArray preprocessedCode = contents.toLatin1(); // ### use the QtCreator C++ preprocessor.

    Document::Ptr doc(new Document());
    doc->_currentGlslVersion = version;
    doc->_vulkanEnabled = m_vulkanSupport->isChecked();
    doc->_engine = new Engine();
    Parser parser(doc->_engine, preprocessedCode.constData(), preprocessedCode.size(), variant);

    TranslationUnitAST *ast = parser.parse();
    if (ast || extraSelections(CodeWarningsSelection).isEmpty()) {
        Semantic sem;
        Scope *globalScope = new Namespace();
        doc->_globalScope = globalScope;
        const InitFile *file = shaderInit(variant);
        sem.translationUnit(file->ast(), globalScope, file->engine());
        if (variant & Lexer::Variant_VertexShader) {
            file = vertexShaderInit(variant);
            sem.translationUnit(file->ast(), globalScope, file->engine());
        }
        if (variant & Lexer::Variant_FragmentShader) {
            file = fragmentShaderInit(variant);
            sem.translationUnit(file->ast(), globalScope, file->engine());
        }
        if (variant & Lexer::Variant_Vulkan) {
            file = vulkanInit(variant);
            sem.translationUnit(file->ast(), globalScope, file->engine());
        }
        sem.translationUnit(ast, globalScope, doc->_engine);

        CreateRangesMarkSemanticDetails createRanges(document(), doc, globalScope->asNamespace());
        createRanges(ast);

#if 0
        QTextStream qout(stdout, QIODevice::WriteOnly);
        GLSL::ASTDump dump(qout);
        dump(ast);
#endif
        const TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::fontSettings();

        QTextCharFormat warningFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
        QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

        QList<QTextEdit::ExtraSelection> sels;
        QSet<int> errors;
        QSet<DiagnosticMessage::Location> errorsWithLocations;

        const QList<DiagnosticMessage> messages = doc->_engine->diagnosticMessages();
        for (const DiagnosticMessage &m : messages) {
            if (! m.line())
                continue;
            if (!Utils::insert(errorsWithLocations, m.location())) {
                if (!Utils::insert(errors, m.line()))
                    continue;
            }

            QTextEdit::ExtraSelection sel;
            sel.cursor = cursorForDiagnosticMessage(m);
            sel.format = m.isError() ? errorFormat : warningFormat;
            sel.format.setToolTip(m.message());
            sels.append(sel);
        }

        setExtraSelections(CodeWarningsSelection, sels);
        setExtraSelections(OtherSelection, createRanges.markedSemantics());
        m_glslDocument = doc;
    }
}

static QStringList diagnosticMessagesToStringList(const QList<DiagnosticMessage> &dMessages,
                                                  int lineNo, int pos)
{
    QStringList fullLine;
    QStringList specific;
    for (const DiagnosticMessage &msg : dMessages) {
        if (lineNo != msg.line())
            continue;
        const DiagnosticMessage::Location &loc = msg.location();
        if (loc.length == -1)
            fullLine.append(msg.message());
        else if (loc.position <= pos && loc.position + loc.length >= pos)
            specific.append(msg.message());
    }
    if (fullLine.isEmpty())
        return specific;
    return specific + fullLine;
}

void GlslEditorWidget::onTooltipRequested(const QPoint &point, int pos)
{
    QTC_ASSERT(m_glslDocument && m_glslDocument->engine(), return);
    const int lineno = document()->findBlock(pos).blockNumber() + 1;
    const QStringList messages = diagnosticMessagesToStringList(
                m_glslDocument->engine()->diagnosticMessages(), lineno, pos);
    if (!messages.isEmpty())
        Utils::ToolTip::show(point, messages.join("<hr/>"), this);
    else
        Utils::ToolTip::hide();
}

int languageVariant(const QString &type)
{
    int variant = 0;
    bool isVertex = false;
    bool isFragment = false;
    bool isDesktop = false;
    if (type.isEmpty()) {
        // ### Before file has been opened, so don't know the mime type.
        isVertex = true;
        isFragment = true;
    } else if (type == QLatin1String("text/x-glsl") ||
               type == QLatin1String(Utils::Constants::GLSL_MIMETYPE)) {
        isVertex = true;
        isFragment = true;
        isDesktop = true;
    } else if (type == QLatin1String(Utils::Constants::GLSL_VERT_MIMETYPE)) {
        isVertex = true;
        isDesktop = true;
    } else if (type == QLatin1String(Utils::Constants::GLSL_FRAG_MIMETYPE)) {
        isFragment = true;
        isDesktop = true;
    } else if (type == QLatin1String(Utils::Constants::GLSL_ES_VERT_MIMETYPE)) {
        isVertex = true;
    } else if (type == QLatin1String(Utils::Constants::GLSL_ES_FRAG_MIMETYPE)) {
        isFragment = true;
    } else if (type == QLatin1String(Utils::Constants::GLSL_COMP_MIMETYPE)) {
        isFragment = true; // not really, but we define the respective variables/functions there
    } else if (type == QLatin1String(Utils::Constants::GLSL_TESS_MIMETYPE)) {
        isVertex = true; // not really, but we define the respective variables/functions there
    } else if (type == QLatin1String(Utils::Constants::GLSL_GEOM_MIMETYPE)) {
        isVertex = true; // not really, but we define the respective variables/functions there
    }
    if (isDesktop)
        variant |= Lexer::Variant_GLSL_120;
    else
        variant |= Lexer::Variant_GLSL_ES_100;
    if (isVertex)
        variant |= Lexer::Variant_VertexShader;
    if (isFragment)
        variant |= Lexer::Variant_FragmentShader;
    return variant;
}

std::unique_ptr<AssistInterface> GlslEditorWidget::createAssistInterface(
    AssistKind kind, AssistReason reason) const
{
    if (kind != Completion)
        return TextEditorWidget::createAssistInterface(kind, reason);

    return std::make_unique<GlslCompletionAssistInterface>(textCursor(),
                                                           textDocument()->filePath(),
                                                           reason,
                                                           textDocument()->mimeType(),
                                                           m_glslDocument);
}

//  GlslEditorFactory

class GlslEditorFactory final : public TextEditor::TextEditorFactory
{
public:
    GlslEditorFactory()
    {
        setId(Constants::C_GLSLEDITOR_ID);
        setDisplayName(Tr::tr("GLSL Editor"));
        addMimeType(Utils::Constants::GLSL_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_VERT_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_FRAG_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_ES_VERT_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_ES_FRAG_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_COMP_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_TESS_MIMETYPE);
        addMimeType(Utils::Constants::GLSL_GEOM_MIMETYPE);

        setDocumentCreator([]() { return new TextDocument(Constants::C_GLSLEDITOR_ID); });
        setEditorWidgetCreator([]() { return new GlslEditorWidget; });
        setIndenterCreator(&createGlslIndenter);
        setSyntaxHighlighterCreator(&createGlslHighlighter);
        setCommentDefinition(Utils::CommentDefinition::CppStyle);
        setCompletionAssistProvider(createGlslCompletionAssistProvider());
        setParenthesesMatchingEnabled(true);
        setCodeFoldingSupported(true);

        setOptionalActionMask(OptionalActions::Format
                                | OptionalActions::UnCommentSelection
                                | OptionalActions::UnCollapseAll);
    }
};

void setupGlslEditorFactory()
{
    static GlslEditorFactory theGlslEditorFactory;
}

} // GlslEditor::Internal
