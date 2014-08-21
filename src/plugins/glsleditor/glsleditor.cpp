/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"
#include "glslhighlighter.h"
#include "glslautocompleter.h"
#include "glslcompletionassist.h"
#include "glslindenter.h"

#include <glsl/glsllexer.h>
#include <glsl/glslparser.h>
#include <glsl/glslengine.h>
#include <glsl/glslsemantic.h>
#include <glsl/glslsymbols.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/mimedatabase.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <texteditor/refactoroverlay.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <qmldesigner/qmldesignerconstants.h>

#include <utils/changeset.h>
#include <utils/qtcassert.h>
#include <utils/uncommentselection.h>

#include <QCoreApplication>
#include <QSettings>
#include <QComboBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QTextBlock>
#include <QTimer>
#include <QTreeView>

using namespace TextEditor;
using namespace GLSL;
using namespace GLSLEditor::Constants;

namespace GLSLEditor {
namespace Internal {

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150
};

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

GlslEditorWidget::GlslEditorWidget()
{
    setAutoCompleter(new GlslCompleter);
    m_outlineCombo = 0;
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);

    m_updateDocumentTimer.setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer.setSingleShot(true);
    connect(&m_updateDocumentTimer, &QTimer::timeout,
            this, &GlslEditorWidget::updateDocumentNow);

    connect(this, &QPlainTextEdit::textChanged,
            [this]() { m_updateDocumentTimer.start(); });

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

    insertExtraToolBarWidget(BaseTextEditorWidget::Left, m_outlineCombo);

//    if (m_modelManager) {
//        m_semanticHighlighter->setModelManager(m_modelManager);
//        connect(m_modelManager, SIGNAL(documentUpdated(GLSL::Document::Ptr)),
//                this, SLOT(onDocumentUpdated(GLSL::Document::Ptr)));
//        connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,GLSL::LibraryInfo)),
//                this, SLOT(forceSemanticRehighlight()));
//        connect(this->document(), SIGNAL(modificationChanged(bool)), this, SLOT(modificationChanged(bool)));
//    }
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

bool GlslEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    textDocument()->setMimeType(Core::MimeDatabase::findByFile(QFileInfo(fileName)).type());
    bool b = BaseTextEditor::open(errorString, fileName, realFileName);
    return b;
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

BaseTextEditor *GlslEditorWidget::createEditor()
{
    QTC_ASSERT("should not happen anymore" && false, return 0);
}

void GlslEditorWidget::updateDocumentNow()
{
    m_updateDocumentTimer.stop();

    int variant = languageVariant(textDocument()->mimeType());
    const QString contents = toPlainText(); // get the code from the editor
    const QByteArray preprocessedCode = contents.toLatin1(); // ### use the QtCreator C++ preprocessor.

    Document::Ptr doc(new Document());
    Engine *engine = new GLSL::Engine();
    doc->_engine = new GLSL::Engine();
    Parser parser(doc->_engine, preprocessedCode.constData(), preprocessedCode.size(), variant);
    TranslationUnitAST *ast = parser.parse();
    if (ast != 0 || extraSelections(CodeWarningsSelection).isEmpty()) {
        Semantic sem;
        Scope *globalScope = engine->newNamespace();
        doc->_globalScope = globalScope;
        const GlslEditorPlugin::InitFile *file = GlslEditorPlugin::shaderInit(variant);
        sem.translationUnit(file->ast, globalScope, file->engine);
        if (variant & Lexer::Variant_VertexShader) {
            file = GlslEditorPlugin::vertexShaderInit(variant);
            sem.translationUnit(file->ast, globalScope, file->engine);
        }
        if (variant & Lexer::Variant_FragmentShader) {
            file = GlslEditorPlugin::fragmentShaderInit(variant);
            sem.translationUnit(file->ast, globalScope, file->engine);
        }
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

int GlslEditorWidget::languageVariant(const QString &type)
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

IAssistInterface *GlslEditorWidget::createAssistInterface(
    AssistKind kind, AssistReason reason) const
{
    if (kind == Completion)
        return new GlslCompletionAssistInterface(document(),
                                                 position(),
                                                 editor()->document()->filePath(),
                                                 reason,
                                                 textDocument()->mimeType(),
                                                 m_glslDocument);
    return BaseTextEditorWidget::createAssistInterface(kind, reason);
}


//////////////////////////////////////////////////////////////////
//
//  GlslEditor
//
//////////////////////////////////////////////////////////////////

GlslEditor::GlslEditor()
{
    addContext(C_GLSLEDITOR_ID);
    setDuplicateSupported(true);
    setCommentStyle(Utils::CommentDefinition::CppStyle);
    setCompletionAssistProvider(ExtensionSystem::PluginManager::getObject<GlslCompletionAssistProvider>());

    setEditorCreator([]() { return new GlslEditor; });
    setWidgetCreator([]() { return new GlslEditorWidget; });

    setDocumentCreator([]() -> BaseTextDocument * {
        auto doc = new BaseTextDocument(C_GLSLEDITOR_ID);
        doc->setIndenter(new GlslIndenter);
        new Highlighter(doc);
        return doc;
    });
}


//////////////////////////////////////////////////////////////////
//
//  GlslEditorFactory
//
//////////////////////////////////////////////////////////////////

GlslEditorFactory::GlslEditorFactory()
{
    setId(C_GLSLEDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", C_GLSLEDITOR_DISPLAY_NAME));
    addMimeType(GLSL_MIMETYPE);
    addMimeType(GLSL_MIMETYPE_VERT);
    addMimeType(GLSL_MIMETYPE_FRAG);
    addMimeType(GLSL_MIMETYPE_VERT_ES);
    addMimeType(GLSL_MIMETYPE_FRAG_ES);
    new TextEditorActionHandler(this, C_GLSLEDITOR_ID,
                                TextEditorActionHandler::Format
                                | TextEditorActionHandler::UnCommentSelection
                                | TextEditorActionHandler::UnCollapseAll);

}

Core::IEditor *GlslEditorFactory::createEditor()
{
    return new GlslEditor;
}

} // namespace Internal
} // namespace GLSLEditor
