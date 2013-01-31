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

#include "glsleditor.h"
#include "glsleditoreditable.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"
#include "glslhighlighter.h"
#include "glslautocompleter.h"
#include "glslindenter.h"
#include "glslcompletionassist.h"

#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslengine.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/refactoroverlay.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <utils/changeset.h>
#include <utils/uncommentselection.h>

#include <QFileInfo>
#include <QSignalMapper>
#include <QTimer>
#include <QDebug>

#include <QMenu>
#include <QComboBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QToolBar>
#include <QTreeView>
#include <QSharedPointer>

using namespace GLSL;
using namespace GLSLEditor;
using namespace GLSLEditor::Internal;

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150
};

namespace {

class CreateRanges: protected GLSL::Visitor
{
    QTextDocument *textDocument;
    Document::Ptr glslDocument;

public:
    CreateRanges(QTextDocument *textDocument, Document::Ptr glslDocument)
        : textDocument(textDocument), glslDocument(glslDocument) {}

    void operator()(GLSL::AST *ast) { accept(ast); }

protected:
    using GLSL::Visitor::visit;

    virtual void endVisit(GLSL::CompoundStatementAST *ast)
    {
        if (ast->symbol) {
            QTextCursor tc(textDocument);
            tc.setPosition(ast->start);
            tc.setPosition(ast->end, QTextCursor::KeepAnchor);
            glslDocument->addRange(tc, ast->symbol);
        }
    }
};

} // end of anonymous namespace

Document::Document()
    : _engine(0)
    , _ast(0)
    , _globalScope(0)
{

}

Document::~Document()
{
    delete _globalScope;
    delete _engine;
}

GLSL::Scope *Document::scopeAt(int position) const
{
    foreach (const Range &c, _cursors) {
        if (position >= c.cursor.selectionStart() && position <= c.cursor.selectionEnd())
            return c.scope;
    }
    return _globalScope;
}

void Document::addRange(const QTextCursor &cursor, GLSL::Scope *scope)
{
    Range c;
    c.cursor = cursor;
    c.scope = scope;
    _cursors.append(c);
}

GLSLTextEditorWidget::GLSLTextEditorWidget(QWidget *parent) :
    TextEditor::BaseTextEditorWidget(parent),
    m_outlineCombo(0)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setIndenter(new GLSLIndenter());
    setAutoCompleter(new GLSLCompleter());

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));

    new Highlighter(baseTextDocument().data());

//    if (m_modelManager) {
//        m_semanticHighlighter->setModelManager(m_modelManager);
//        connect(m_modelManager, SIGNAL(documentUpdated(GLSL::Document::Ptr)),
//                this, SLOT(onDocumentUpdated(GLSL::Document::Ptr)));
//        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,GLSL::LibraryInfo)),
//                this, SLOT(forceSemanticRehighlight()));
//        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
//    }
}

GLSLTextEditorWidget::~GLSLTextEditorWidget()
{
}

int GLSLTextEditorWidget::editorRevision() const
{
    //return document()->revision();
    return 0;
}

bool GLSLTextEditorWidget::isOutdated() const
{
//    if (m_semanticInfo.revision() != editorRevision())
//        return true;

    return false;
}

Core::IEditor *GLSLEditorEditable::duplicate(QWidget *parent)
{
    GLSLTextEditorWidget *newEditor = new GLSLTextEditorWidget(parent);
    newEditor->duplicateFrom(editorWidget());
    GLSLEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editor();
}

Core::Id GLSLEditorEditable::id() const
{
    return Core::Id(GLSLEditor::Constants::C_GLSLEDITOR_ID);
}

bool GLSLEditorEditable::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    editorWidget()->setMimeType(Core::ICore::mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    bool b = TextEditor::BaseTextEditor::open(errorString, fileName, realFileName);
    return b;
}

void GLSLTextEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    Highlighter *highlighter = qobject_cast<Highlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    /*
        NumberFormat,
        StringFormat,
        TypeFormat,
        KeywordFormat,
        LabelFormat,
        CommentFormat,
        VisualWhitespace,
     */
    static QVector<TextEditor::TextStyle> categories;
    if (categories.isEmpty()) {
        categories << TextEditor::C_NUMBER
                   << TextEditor::C_STRING
                   << TextEditor::C_TYPE
                   << TextEditor::C_KEYWORD
                   << TextEditor::C_OPERATOR
                   << TextEditor::C_PREPROCESSOR
                   << TextEditor::C_LABEL
                   << TextEditor::C_COMMENT
                   << TextEditor::C_DOXYGEN_COMMENT
                   << TextEditor::C_DOXYGEN_TAG
                   << TextEditor::C_VISUAL_WHITESPACE
                   << TextEditor::C_REMOVED_LINE;
    }

    highlighter->setFormats(fs.toTextCharFormats(categories));
    highlighter->rehighlight();
}

QString GLSLTextEditorWidget::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    const QChar ch = characterAt(tc.position() - 1);
    // make sure that we're not at the start of the next word.
    if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
        tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word = tc.selectedText();
    return word;
}

TextEditor::BaseTextEditor *GLSLTextEditorWidget::createEditor()
{
    GLSLEditorEditable *editable = new GLSLEditorEditable(this);
    createToolBar(editable);
    return editable;
}

void GLSLTextEditorWidget::createToolBar(GLSLEditorEditable *editor)
{
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);

    // ### m_outlineCombo->setModel(m_outlineModel);

    QTreeView *treeView = new QTreeView;
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

    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, m_outlineCombo);
}

bool GLSLTextEditorWidget::event(QEvent *e)
{
    return BaseTextEditorWidget::event(e);
}

void GLSLTextEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

void GLSLTextEditorWidget::updateDocument()
{
    m_updateDocumentTimer->start();
}

void GLSLTextEditorWidget::updateDocumentNow()
{
    m_updateDocumentTimer->stop();

    int variant = languageVariant(mimeType());
    const QString contents = toPlainText(); // get the code from the editor
    const QByteArray preprocessedCode = contents.toLatin1(); // ### use the QtCreator C++ preprocessor.

    Document::Ptr doc(new Document());
    GLSL::Engine *engine = new GLSL::Engine();
    doc->_engine = new GLSL::Engine();
    Parser parser(doc->_engine, preprocessedCode.constData(), preprocessedCode.size(), variant);
    TranslationUnitAST *ast = parser.parse();
    if (ast != 0 || extraSelections(CodeWarningsSelection).isEmpty()) {
        GLSLEditorPlugin *plugin = GLSLEditorPlugin::instance();

        Semantic sem;
        Scope *globalScope = engine->newNamespace();
        doc->_globalScope = globalScope;
        sem.translationUnit(plugin->shaderInit(variant)->ast, globalScope, plugin->shaderInit(variant)->engine);
        if (variant & Lexer::Variant_VertexShader)
            sem.translationUnit(plugin->vertexShaderInit(variant)->ast, globalScope, plugin->vertexShaderInit(variant)->engine);
        if (variant & Lexer::Variant_FragmentShader)
            sem.translationUnit(plugin->fragmentShaderInit(variant)->ast, globalScope, plugin->fragmentShaderInit(variant)->engine);
        sem.translationUnit(ast, globalScope, engine);

        CreateRanges createRanges(document(), doc);
        createRanges(ast);

        QTextCharFormat errorFormat;
        errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        errorFormat.setUnderlineColor(Qt::red);

        QTextCharFormat warningFormat;
        warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        warningFormat.setUnderlineColor(Qt::darkYellow);

        QList<QTextEdit::ExtraSelection> sels;
        QSet<int> errors;

        foreach (const DiagnosticMessage &m, engine->diagnosticMessages()) {
            if (! m.line())
                continue;
            else if (errors.contains(m.line()))
                continue;

            errors.insert(m.line());

            QTextCursor cursor(document()->findBlockByNumber(m.line() - 1));
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.cursor = cursor;
            sel.format = m.isError() ? errorFormat : warningFormat;
            sel.format.setToolTip(m.message());
            sels.append(sel);
        }

        setExtraSelections(CodeWarningsSelection, sels);
        m_glslDocument = doc;
    }
}

int GLSLTextEditorWidget::languageVariant(const QString &type)
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
               type == QLatin1String("application/x-glsl")) {
        isVertex = true;
        isFragment = true;
        isDesktop = true;
    } else if (type == QLatin1String("text/x-glsl-vert")) {
        isVertex = true;
        isDesktop = true;
    } else if (type == QLatin1String("text/x-glsl-frag")) {
        isFragment = true;
        isDesktop = true;
    } else if (type == QLatin1String("text/x-glsl-es-vert")) {
        isVertex = true;
    } else if (type == QLatin1String("text/x-glsl-es-frag")) {
        isFragment = true;
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

Document::Ptr GLSLTextEditorWidget::glslDocument() const
{
    return m_glslDocument;
}

TextEditor::IAssistInterface *GLSLTextEditorWidget::createAssistInterface(
    TextEditor::AssistKind kind,
    TextEditor::AssistReason reason) const
{
    if (kind == TextEditor::Completion)
        return new GLSLCompletionAssistInterface(document(),
                                                 position(),
                                                 editor()->document(),
                                                 reason,
                                                 mimeType(),
                                                 glslDocument());
    return BaseTextEditorWidget::createAssistInterface(kind, reason);
}
