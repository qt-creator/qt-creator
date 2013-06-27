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

#include "cppeditor.h"

#include "cppeditorconstants.h"
#include "cppeditorplugin.h"
#include "cpphighlighter.h"
#include "cppautocompleter.h"
#include "cppquickfixassistant.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <cpptools/cpptoolseditorsupport.h>
#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppchecksymbols.h>
#include <cpptools/cppcodeformatter.h>
#include <cpptools/cppcompletionsupport.h>
#include <cpptools/cpphighlightingsupport.h>
#include <cpptools/cpplocalsymbols.h>
#include <cpptools/cppqtstyleindenter.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/doxygengenerator.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/symbolfinder.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>

#include <utils/qtcassert.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/OverviewModel.h>
#include <cplusplus/BackwardsScanner.h>

#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QSignalMapper>
#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QTextEdit>
#include <QComboBox>
#include <QTreeView>
#include <QSortFilterProxyModel>

enum {
    UPDATE_OUTLINE_INTERVAL = 500,
    UPDATE_USES_INTERVAL = 500,
    UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200
};

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppEditor::Internal;

namespace {

class OverviewTreeView : public QTreeView
{
public:
    OverviewTreeView(QWidget *parent = 0)
        : QTreeView(parent)
    {
        // TODO: Disable the root for all items (with a custom delegate?)
        setRootIsDecorated(false);
    }

    void sync()
    {
        expandAll();
    }

    void adjustWidth()
    {
        const int w = Core::ICore::mainWindow()->geometry().width();
        setMaximumWidth(w);
        setMinimumWidth(qMin(qMax(sizeHintForColumn(0), minimumSizeHint().width()), w));
    }
};

class OverviewCombo : public QComboBox
{
public:
    OverviewCombo(QWidget *parent = 0) : QComboBox(parent), m_skipNextHide(false)
    {}

    bool eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::MouseButtonPress && object == view()->viewport()) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QModelIndex index = view()->indexAt(mouseEvent->pos());
            if (!view()->visualRect(index).contains(mouseEvent->pos()))
                m_skipNextHide = true;
        }
        return false;
    }

    void showPopup()
    {
        static_cast<OverviewTreeView *>(view())->adjustWidth();
        QComboBox::showPopup();
    }

    virtual void hidePopup()
    {
        if (m_skipNextHide)
            m_skipNextHide = false;
        else
            QComboBox::hidePopup();
    }

private:
    bool m_skipNextHide;
};

class OverviewProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    OverviewProxyModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent) :
        QSortFilterProxyModel(parent),
        m_sourceModel(sourceModel)
    {
        setSourceModel(m_sourceModel);
    }

    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
    {
        // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
        const QModelIndex sourceIndex = m_sourceModel->index(sourceRow, 0, sourceParent);
        CPlusPlus::Symbol *symbol = m_sourceModel->symbolFromIndex(sourceIndex);
        if (symbol && symbol->isGenerated())
            return false;

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
private:
    CPlusPlus::OverviewModel *m_sourceModel;
};

class FindFunctionDefinitions: protected SymbolVisitor
{
    const Name *_declarationName;
    QList<Function *> *_functions;

public:
    FindFunctionDefinitions()
        : _declarationName(0),
          _functions(0)
    { }

    void operator()(const Name *declarationName, Scope *globals,
                    QList<Function *> *functions)
    {
        _declarationName = declarationName;
        _functions = functions;

        for (unsigned i = 0; i < globals->memberCount(); ++i) {
            accept(globals->memberAt(i));
        }
    }

protected:
    using SymbolVisitor::visit;

    virtual bool visit(Function *function)
    {
        const Name *name = function->name();
        if (const QualifiedNameId *q = name->asQualifiedNameId())
            name = q->name();

        if (_declarationName->isEqualTo(name))
            _functions->append(function);

        return false;
    }
};

struct CanonicalSymbol
{
    CPPEditorWidget *editor;
    TypeOfExpression typeOfExpression;
    SemanticInfo info;

    CanonicalSymbol(CPPEditorWidget *editor, const SemanticInfo &info)
        : editor(editor), info(info)
    {
        typeOfExpression.init(info.doc, info.snapshot);
        typeOfExpression.setExpandTemplates(true);
    }

    const LookupContext &context() const
    {
        return typeOfExpression.context();
    }

    static inline bool isIdentifierChar(const QChar &ch)
    {
        return ch.isLetterOrNumber() || ch == QLatin1Char('_');
    }

    Scope *getScopeAndExpression(const QTextCursor &cursor, QString *code)
    {
        return getScopeAndExpression(editor, info, cursor, code);
    }

    static Scope *getScopeAndExpression(CPPEditorWidget *editor, const SemanticInfo &info,
                                        const QTextCursor &cursor,
                                        QString *code)
    {
        if (! info.doc)
            return 0;

        QTextCursor tc = cursor;
        int line, col;
        editor->convertPosition(tc.position(), &line, &col);
        ++col; // 1-based line and 1-based column

        QTextDocument *document = editor->document();

        int pos = tc.position();

        if (! isIdentifierChar(document->characterAt(pos)))
            if (! (pos > 0 && isIdentifierChar(document->characterAt(pos - 1))))
                return 0;

        while (isIdentifierChar(document->characterAt(pos)))
            ++pos;
        tc.setPosition(pos);

        ExpressionUnderCursor expressionUnderCursor;
        *code = expressionUnderCursor(tc);
        return info.doc->scopeAt(line, col);
    }

    Symbol *operator()(const QTextCursor &cursor)
    {
        QString code;

        if (Scope *scope = getScopeAndExpression(cursor, &code))
            return operator()(scope, code);

        return 0;
    }

    Symbol *operator()(Scope *scope, const QString &code)
    {
        return canonicalSymbol(scope, code, typeOfExpression);
    }

    static Symbol *canonicalSymbol(Scope *scope, const QString &code,
                                   TypeOfExpression &typeOfExpression)
    {
        const QList<LookupItem> results =
                typeOfExpression(code.toUtf8(), scope, TypeOfExpression::Preprocess);

        for (int i = results.size() - 1; i != -1; --i) {
            const LookupItem &r = results.at(i);
            Symbol *decl = r.declaration();

            if (! (decl && decl->enclosingScope()))
                break;

            if (Class *classScope = r.declaration()->enclosingScope()->asClass()) {
                const Identifier *declId = decl->identifier();
                const Identifier *classId = classScope->identifier();

                if (classId && classId->isEqualTo(declId))
                    continue; // skip it, it's a ctor or a dtor.

                else if (Function *funTy = r.declaration()->type()->asFunctionType()) {
                    if (funTy->isVirtual())
                        return r.declaration();
                }
            }
        }

        for (int i = 0; i < results.size(); ++i) {
            const LookupItem &r = results.at(i);

            if (r.declaration())
                return r.declaration();
        }

        return 0;
    }

};

int numberOfClosedEditors = 0;

/// Check if previous line is a CppStyle Doxygen Comment
bool isPreviousLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.previous();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return true;

    return false;
}

/// Check if next line is a CppStyle Doxygen Comment
bool isNextLineCppStyleComment(const QTextCursor &cursor)
{
    const QTextBlock &currentBlock = cursor.block();
    if (!currentBlock.isValid())
        return false;

    const QTextBlock &actual = currentBlock.next();
    if (!actual.isValid())
        return false;

    const QString text = actual.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return true;

    return false;
}

/// Check if line is a CppStyle Doxygen comment and the cursor is after the comment
bool isCursorAfterCppComment(const QTextCursor &cursor, const QTextDocument *doc)
{
    QTextCursor cursorFirstNonBlank(cursor);
    cursorFirstNonBlank.movePosition(QTextCursor::StartOfLine);
    while (doc->characterAt(cursorFirstNonBlank.position()).isSpace()
           && cursorFirstNonBlank.movePosition(QTextCursor::NextCharacter)) {
    }

    const QTextBlock& block = cursorFirstNonBlank.block();
    const QString text = block.text().trimmed();
    if (text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!")))
        return (cursor.position() >= cursorFirstNonBlank.position() + 3);

    return false;
}

bool isCppStyleContinuation(const QTextCursor& cursor)
{
    return (isPreviousLineCppStyleComment(cursor) || isNextLineCppStyleComment(cursor));
}

DoxygenGenerator::DocumentationStyle doxygenStyle(const QTextCursor &cursor,
                                                  const QTextDocument *doc)
{
    const int pos = cursor.position();

    QString comment = QString(doc->characterAt(pos - 3))
            + doc->characterAt(pos - 2)
            + doc->characterAt(pos - 1);

    if (comment == QLatin1String("/**"))
        return CppTools::DoxygenGenerator::JavaStyle;
    else if (comment == QLatin1String("/*!"))
        return CppTools::DoxygenGenerator::QtStyle;
    else if (comment == QLatin1String("///"))
        return CppTools::DoxygenGenerator::CppStyleA;
    else
        return CppTools::DoxygenGenerator::CppStyleB;
}

bool handleDoxygenCppStyleContinuation(QTextCursor &cursor,
                                       QKeyEvent *e)
{
    const int blockPos = cursor.positionInBlock();
    const QString &text = cursor.block().text();
    int offset = 0;
    for (; offset < blockPos; ++offset) {
        if (!text.at(offset).isSpace())
            break;
    }

    // If the line does not start with the comment we don't
    // consider it as a continuation. Handles situations like:
    // void d(); ///<enter>
    if (!(text.trimmed().startsWith(QLatin1String("///"))
          || text.startsWith(QLatin1String("//!")))) {
        return false;
    }

    QString newLine(QLatin1Char('\n'));
    newLine.append(QString(offset, QLatin1Char(' '))); // indent correctly

    const QString commentMarker = text.mid(offset, 3);
    newLine.append(commentMarker);

    cursor.insertText(newLine);
    e->accept();
    return true;
}

bool handleDoxygenContinuation(QTextCursor &cursor,
                               QKeyEvent *e,
                               const QTextDocument *doc,
                               const bool enableDoxygen,
                               const bool leadingAsterisks)
{
    // It might be a continuation if:
    // a) current line starts with /// or //! and cursor is positioned after the comment
    // b) current line is in the middle of a multi-line Qt or Java style comment

    if (enableDoxygen && !cursor.atEnd() && isCursorAfterCppComment(cursor, doc))
        return handleDoxygenCppStyleContinuation(cursor, e);

    if (!leadingAsterisks)
        return false;

    // We continue the comment if the cursor is after a comment's line asterisk and if
    // there's no asterisk immediately after the cursor (that would already be considered
    // a leading asterisk).
    int offset = 0;
    const int blockPos = cursor.positionInBlock();
    const QString &text = cursor.block().text();
    for (; offset < blockPos; ++offset) {
        if (!text.at(offset).isSpace())
            break;
    }

    if (offset < blockPos
            && (text.at(offset) == QLatin1Char('*')
                || (offset < blockPos - 1
                    && text.at(offset) == QLatin1Char('/')
                    && text.at(offset + 1) == QLatin1Char('*')))) {
        int followinPos = blockPos;
        for (; followinPos < text.length(); ++followinPos) {
            if (!text.at(followinPos).isSpace())
                break;
        }
        if (followinPos == text.length()
                || text.at(followinPos) != QLatin1Char('*')) {
            QString newLine(QLatin1Char('\n'));
            QTextCursor c(cursor);
            c.movePosition(QTextCursor::StartOfBlock);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, offset);
            newLine.append(c.selectedText());
            if (text.at(offset) == QLatin1Char('/')) {
                newLine.append(QLatin1String(" *"));
            } else {
                int start = offset;
                while (offset < blockPos && text.at(offset) == QLatin1Char('*'))
                    ++offset;
                newLine.append(QString(offset - start, QLatin1Char('*')));
            }
            cursor.insertText(newLine);
            e->accept();
            return true;
        }
    }

    return false;
}

} // end of anonymous namespace

CPPEditor::CPPEditor(CPPEditorWidget *editor)
    : BaseTextEditor(editor)
{
    m_context.add(CppEditor::Constants::C_CPPEDITOR);
    m_context.add(ProjectExplorer::Constants::LANG_CXX);
    m_context.add(TextEditor::Constants::C_TEXTEDITOR);
}

Q_GLOBAL_STATIC(CppTools::SymbolFinder, symbolFinder)

CPPEditorWidget::CPPEditorWidget(QWidget *parent)
    : TextEditor::BaseTextEditorWidget(parent)
    , m_currentRenameSelection(NoCurrentRenameSelection)
    , m_inRename(false)
    , m_inRenameChanged(false)
    , m_firstRenameChange(false)
    , m_objcEnabled(false)
    , m_commentsSettings(CppTools::CppToolsSettings::instance()->commentsSettings())
    , m_completionSupport(0)
{
    qRegisterMetaType<SemanticInfo>("CppTools::SemanticInfo");

    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setIndenter(new CppTools::CppQtStyleIndenter);
    setAutoCompleter(new CppAutoCompleter);

    baseTextDocument()->setSyntaxHighlighter(new CppHighlighter);

    m_modelManager = CppModelManagerInterface::instance();
    if (m_modelManager) {
        CppEditorSupport *editorSupport = m_modelManager->cppEditorSupport(editor());
        connect(editorSupport, SIGNAL(documentUpdated()),
                this, SLOT(onDocumentUpdated()));
        connect(editorSupport, SIGNAL(semanticInfoUpdated(CppTools::SemanticInfo)),
                this, SLOT(updateSemanticInfo(CppTools::SemanticInfo)));
        connect(editorSupport, SIGNAL(highlighterStarted(QFuture<TextEditor::HighlightingResult>*,uint)),
                this, SLOT(highlighterStarted(QFuture<TextEditor::HighlightingResult>*,uint)));

        m_completionSupport = m_modelManager->completionSupport(editor());
    }

    m_highlightRevision = 0;
    connect(&m_highlightWatcher, SIGNAL(resultsReadyAt(int,int)),
            SLOT(highlightSymbolUsages(int,int)));
    connect(&m_highlightWatcher, SIGNAL(finished()),
            SLOT(finishHighlightSymbolUsages()));

    m_referencesRevision = 0;
    m_referencesCursorPosition = 0;
    connect(&m_referencesWatcher, SIGNAL(finished()), SLOT(markSymbolsNow()));

    connect(this, SIGNAL(refactorMarkerClicked(TextEditor::RefactorMarker)),
            this, SLOT(onRefactorMarkerClicked(TextEditor::RefactorMarker)));

    m_declDefLinkFinder = new FunctionDeclDefLinkFinder(this);
    connect(m_declDefLinkFinder, SIGNAL(foundLink(QSharedPointer<FunctionDeclDefLink>)),
            this, SLOT(onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink>)));

    connect(CppTools::CppToolsSettings::instance(),
            SIGNAL(commentsSettingsChanged(CppTools::CommentsSettings)),
            this,
            SLOT(onCommentsSettingsChanged(CppTools::CommentsSettings)));
}

CPPEditorWidget::~CPPEditorWidget()
{
    if (m_modelManager)
        m_modelManager->deleteEditorSupport(editor());

    ++numberOfClosedEditors;
    if (numberOfClosedEditors == 5) {
        if (m_modelManager)
            m_modelManager->GC();
        numberOfClosedEditors = 0;
    }

    delete m_completionSupport;
}

TextEditor::BaseTextEditor *CPPEditorWidget::createEditor()
{
    CPPEditor *editable = new CPPEditor(this);
    createToolBar(editable);
    return editable;
}

void CPPEditorWidget::createToolBar(CPPEditor *editor)
{
    m_outlineCombo = new OverviewCombo;
    m_outlineCombo->setMinimumContentsLength(22);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_outlineCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_outlineCombo->setSizePolicy(policy);

    QTreeView *outlineView = new OverviewTreeView;
    outlineView->header()->hide();
    outlineView->setItemsExpandable(true);
    m_outlineCombo->setView(outlineView);
    m_outlineCombo->setMaxVisibleItems(40);
    outlineView->viewport()->installEventFilter(m_outlineCombo);

    m_outlineModel = new OverviewModel(this);
    m_proxyModel = new OverviewProxyModel(m_outlineModel, this);
    if (CppEditorPlugin::instance()->sortedOutline())
        m_proxyModel->sort(0, Qt::AscendingOrder);
    else
        m_proxyModel->sort(-1, Qt::AscendingOrder); // don't sort yet, but set column for sortedOutline()
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_outlineCombo->setModel(m_proxyModel);

    m_outlineCombo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_sortAction = new QAction(tr("Sort Alphabetically"), m_outlineCombo);
    m_sortAction->setCheckable(true);
    m_sortAction->setChecked(sortedOutline());
    connect(m_sortAction, SIGNAL(toggled(bool)),
            CppEditorPlugin::instance(), SLOT(setSortedOutline(bool)));
    m_outlineCombo->addAction(m_sortAction);

    m_updateOutlineTimer = new QTimer(this);
    m_updateOutlineTimer->setSingleShot(true);
    m_updateOutlineTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    connect(m_updateOutlineTimer, SIGNAL(timeout()), this, SLOT(updateOutlineNow()));

    m_updateOutlineIndexTimer = new QTimer(this);
    m_updateOutlineIndexTimer->setSingleShot(true);
    m_updateOutlineIndexTimer->setInterval(UPDATE_OUTLINE_INTERVAL);
    connect(m_updateOutlineIndexTimer, SIGNAL(timeout()), this, SLOT(updateOutlineIndexNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setSingleShot(true);
    m_updateUsesTimer->setInterval(UPDATE_USES_INTERVAL);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    m_updateFunctionDeclDefLinkTimer = new QTimer(this);
    m_updateFunctionDeclDefLinkTimer->setSingleShot(true);
    m_updateFunctionDeclDefLinkTimer->setInterval(UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(m_updateFunctionDeclDefLinkTimer, SIGNAL(timeout()),
            this, SLOT(updateFunctionDeclDefLinkNow()));

    connect(m_outlineCombo, SIGNAL(activated(int)), this, SLOT(jumpToOutlineElement(int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateOutlineIndex()));
    connect(m_outlineCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateOutlineToolTip()));

    // set up slots to document changes
    updateContentsChangedSignal();
    connect(editorDocument(), SIGNAL(changed()), this, SLOT(updateFileName()));

    // set up function declaration - definition link
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateFunctionDeclDefLink()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateFunctionDeclDefLink()));

    // set up the semantic highlighter
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateUses()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));

    editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, m_outlineCombo);
}

void CPPEditorWidget::paste()
{
    if (m_currentRenameSelection == NoCurrentRenameSelection) {
        BaseTextEditorWidget::paste();
        return;
    }

    startRename();
    BaseTextEditorWidget::paste();
    finishRename();
}

void CPPEditorWidget::cut()
{
    if (m_currentRenameSelection == NoCurrentRenameSelection) {
        BaseTextEditorWidget::cut();
        return;
    }

    startRename();
    BaseTextEditorWidget::cut();
    finishRename();
}

void CPPEditorWidget::selectAll()
{
    // if we are currently renaming a symbol
    // and the cursor is over that symbol, select just that symbol
    if (m_currentRenameSelection != NoCurrentRenameSelection) {
        QTextCursor cursor = textCursor();
        int selectionBegin = m_currentRenameSelectionBegin.position();
        int selectionEnd = m_currentRenameSelectionEnd.position();

        if (cursor.position() >= selectionBegin
                && cursor.position() <= selectionEnd) {
            cursor.setPosition(selectionBegin);
            cursor.setPosition(selectionEnd, QTextCursor::KeepAnchor);
            setTextCursor(cursor);
            return;
        }
    }

    BaseTextEditorWidget::selectAll();
}

CppModelManagerInterface *CPPEditorWidget::modelManager() const
{
    return m_modelManager;
}

void CPPEditorWidget::setMimeType(const QString &mt)
{
    BaseTextEditorWidget::setMimeType(mt);
    setObjCEnabled(mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
}

void CPPEditorWidget::setObjCEnabled(bool onoff)
{
    m_objcEnabled = onoff;
}

bool CPPEditorWidget::isObjCEnabled() const
{ return m_objcEnabled; }

void CPPEditorWidget::startRename()
{
    m_inRenameChanged = false;
}

void CPPEditorWidget::finishRename()
{
    if (!m_inRenameChanged)
        return;

    m_inRename = true;

    QTextCursor cursor = textCursor();
    cursor.joinPreviousEditBlock();

    cursor.setPosition(m_currentRenameSelectionEnd.position());
    cursor.setPosition(m_currentRenameSelectionBegin.position(), QTextCursor::KeepAnchor);
    m_renameSelections[m_currentRenameSelection].cursor = cursor;
    QString text = cursor.selectedText();

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        if (i == m_currentRenameSelection)
            continue;
        QTextEdit::ExtraSelection &s = m_renameSelections[i];
        int pos = s.cursor.selectionStart();
        s.cursor.removeSelectedText();
        s.cursor.insertText(text);
        s.cursor.setPosition(pos, QTextCursor::KeepAnchor);
    }

    setExtraSelections(CodeSemanticsSelection, m_renameSelections);
    cursor.endEditBlock();

    m_inRename = false;
}

void CPPEditorWidget::abortRename()
{
    if (m_currentRenameSelection <= NoCurrentRenameSelection)
        return;
    m_renameSelections[m_currentRenameSelection].format = m_occurrencesFormat;
    m_currentRenameSelection = NoCurrentRenameSelection;
    m_currentRenameSelectionBegin = QTextCursor();
    m_currentRenameSelectionEnd = QTextCursor();
    setExtraSelections(CodeSemanticsSelection, m_renameSelections);

    semanticRehighlight(/* force = */ true);
}

/// \brief Called by \c CppEditorSupport when the document corresponding to the
///        file in this editor is updated.
void CPPEditorWidget::onDocumentUpdated()
{
    m_updateOutlineTimer->start();
}

const Macro *CPPEditorWidget::findCanonicalMacro(const QTextCursor &cursor, Document::Ptr doc) const
{
    if (! doc)
        return 0;

    int line, col;
    convertPosition(cursor.position(), &line, &col);

    if (const Macro *macro = doc->findMacroDefinitionAt(line)) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = identifierUnderCursor(&macroCursor).toLatin1();
        if (macro->name() == name)
            return macro;
    } else if (const Document::MacroUse *use = doc->findMacroUseAt(cursor.position())) {
        return &use->macro();
    }

    return 0;
}

void CPPEditorWidget::findUsages()
{
    SemanticInfo info = m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        m_modelManager->findMacroUsages(*macro);
    } else {
        CanonicalSymbol cs(this, info);
        Symbol *canonicalSymbol = cs(textCursor());
        if (canonicalSymbol)
            m_modelManager->findUsages(canonicalSymbol, cs.context());
    }
}


void CPPEditorWidget::renameUsagesNow(const QString &replacement)
{
    SemanticInfo info = m_lastSemanticInfo;
    info.snapshot = CppModelManagerInterface::instance()->snapshot();
    info.snapshot.insert(info.doc);

    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        m_modelManager->renameMacroUsages(*macro, replacement);
    } else {
        CanonicalSymbol cs(this, info);
        if (Symbol *canonicalSymbol = cs(textCursor()))
            if (canonicalSymbol->identifier() != 0)
                m_modelManager->renameUsages(canonicalSymbol, cs.context(), replacement);
    }
}

void CPPEditorWidget::renameUsages()
{
    renameUsagesNow();
}

void CPPEditorWidget::markSymbolsNow()
{
    if (m_references.isCanceled())
        return;
    else if (m_referencesCursorPosition != position())
        return;
    else if (m_referencesRevision != editorRevision())
        return;

    const SemanticInfo info = m_lastSemanticInfo;
    TranslationUnit *unit = info.doc->translationUnit();
    const QList<int> result = m_references.result();

    QList<QTextEdit::ExtraSelection> selections;

    foreach (int index, result) {
        unsigned line, column;
        unit->getTokenPosition(index, &line, &column);

        if (column)
            --column;  // adjust the column position.

        const int len = unit->tokenAt(index).f.length;

        QTextCursor cursor(document()->findBlockByNumber(line - 1));
        cursor.setPosition(cursor.position() + column);
        cursor.setPosition(cursor.position() + len, QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection sel;
        sel.format = m_occurrencesFormat;
        sel.cursor = cursor;
        selections.append(sel);

    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

static QList<int> lazyFindReferences(Scope *scope, QString code, Document::Ptr doc,
                                     Snapshot snapshot)
{
    TypeOfExpression typeOfExpression;
    snapshot.insert(doc);
    typeOfExpression.init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    if (Symbol *canonicalSymbol = CanonicalSymbol::canonicalSymbol(scope, code, typeOfExpression))
        return CppModelManagerInterface::instance()->references(canonicalSymbol,
                                                                typeOfExpression.context());
    return QList<int>();
}

void CPPEditorWidget::markSymbols(const QTextCursor &tc, const SemanticInfo &info)
{
    abortRename();

    if (! info.doc)
        return;

    if (const Macro *macro = findCanonicalMacro(textCursor(), info.doc)) {
        QList<QTextEdit::ExtraSelection> selections;

        //Macro definition
        if (macro->fileName() == info.doc->fileName()) {
            QTextCursor cursor(document());
            cursor.setPosition(macro->offset());
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                macro->name().length());

            QTextEdit::ExtraSelection sel;
            sel.format = m_occurrencesFormat;
            sel.cursor = cursor;
            selections.append(sel);
        }

        //Other macro uses
        foreach (const Document::MacroUse &use, info.doc->macroUses()) {
            if (use.macro().line() != macro->line()
                    || use.macro().offset() != macro->offset()
                    || use.macro().length() != macro->length()
                    || use.macro().fileName() != macro->fileName())
                continue;

            QTextCursor cursor(document());
            cursor.setPosition(use.begin());
            cursor.setPosition(use.end(), QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection sel;
            sel.format = m_occurrencesFormat;
            sel.cursor = cursor;
            selections.append(sel);
        }

        setExtraSelections(CodeSemanticsSelection, selections);
    } else {
        CanonicalSymbol cs(this, info);
        QString expression;
        if (Scope *scope = cs.getScopeAndExpression(this, info, tc, &expression)) {
            m_references.cancel();
            m_referencesRevision = info.revision;
            m_referencesCursorPosition = position();
            m_references = QtConcurrent::run(&lazyFindReferences, scope, expression, info.doc,
                                             info.snapshot);
            m_referencesWatcher.setFuture(m_references);
        } else {
            const QList<QTextEdit::ExtraSelection> selections = extraSelections(CodeSemanticsSelection);

            if (! selections.isEmpty())
                setExtraSelections(CodeSemanticsSelection, QList<QTextEdit::ExtraSelection>());
        }
    }
}

void CPPEditorWidget::renameSymbolUnderCursor()
{
    CppEditorSupport *edSup = m_modelManager->cppEditorSupport(editor());
    updateSemanticInfo(edSup->recalculateSemanticInfo(/* emitSignalWhenFinished = */ false));
    abortRename();

    QTextCursor c = textCursor();

    for (int i = 0; i < m_renameSelections.size(); ++i) {
        QTextEdit::ExtraSelection s = m_renameSelections.at(i);
        if (c.position() >= s.cursor.anchor()
                && c.position() <= s.cursor.position()) {
            m_currentRenameSelection = i;
            m_firstRenameChange = true;
            m_currentRenameSelectionBegin = QTextCursor(c.document()->docHandle(),
                                                        m_renameSelections[i].cursor.selectionStart());
            m_currentRenameSelectionEnd = QTextCursor(c.document()->docHandle(),
                                                      m_renameSelections[i].cursor.selectionEnd());
            m_renameSelections[i].format = m_occurrenceRenameFormat;
            setExtraSelections(CodeSemanticsSelection, m_renameSelections);
            break;
        }
    }

    if (m_renameSelections.isEmpty())
        renameUsages();
}

void CPPEditorWidget::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
    if (m_currentRenameSelection == NoCurrentRenameSelection || m_inRename)
        return;

    if (position + charsAdded == m_currentRenameSelectionBegin.position()) {
        // we are inserting at the beginning of the rename selection => expand
        m_currentRenameSelectionBegin.setPosition(position);
        m_renameSelections[m_currentRenameSelection].cursor.setPosition(position,
                                                                        QTextCursor::KeepAnchor);
    }

    // the condition looks odd, but keep in mind that the begin
    // and end cursors do move automatically
    m_inRenameChanged = (position >= m_currentRenameSelectionBegin.position()
                         && position + charsAdded <= m_currentRenameSelectionEnd.position());

    if (!m_inRenameChanged)
        abortRename();

    if (charsRemoved > 0)
        updateUses();
}

void CPPEditorWidget::updateFileName()
{}

void CPPEditorWidget::jumpToOutlineElement(int index)
{
    QModelIndex modelIndex = m_outlineCombo->view()->currentIndex();
    // When the user clicks on an item in the combo box,
    // the view's currentIndex is updated, so we want to use that.
    // When the scroll wheel was used on the combo box,
    // the view's currentIndex is not updated,
    // but the passed index to this method is correct.
    // So, if the view has a current index, we reset it, to be able
    // to distinguish wheel events later
    if (modelIndex.isValid())
        m_outlineCombo->view()->setCurrentIndex(QModelIndex());
    else
        modelIndex = m_proxyModel->index(index, 0); // toplevel index
    QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);
    Symbol *symbol = m_outlineModel->symbolFromIndex(sourceIndex);
    if (! symbol)
        return;

    openCppEditorAt(linkToSymbol(symbol));
}

void CPPEditorWidget::setSortedOutline(bool sort)
{
    if (sort != sortedOutline()) {
        if (sort)
            m_proxyModel->sort(0, Qt::AscendingOrder);
        else
            m_proxyModel->sort(-1, Qt::AscendingOrder);
        bool block = m_sortAction->blockSignals(true);
        m_sortAction->setChecked(m_proxyModel->sortColumn() == 0);
        m_sortAction->blockSignals(block);
        updateOutlineIndexNow();
    }
}

bool CPPEditorWidget::sortedOutline() const
{
    return (m_proxyModel->sortColumn() == 0);
}

void CPPEditorWidget::updateOutlineNow()
{
    const Snapshot snapshot = m_modelManager->snapshot();
    Document::Ptr document = snapshot.document(editorDocument()->fileName());

    if (!document)
        return;

    if (document->editorRevision() != editorRevision()) {
        m_updateOutlineTimer->start();
        return;
    }

    m_outlineModel->rebuild(document);

    OverviewTreeView *treeView = static_cast<OverviewTreeView *>(m_outlineCombo->view());
    treeView->sync();
    updateOutlineIndexNow();
}

void CPPEditorWidget::updateOutlineIndex()
{
    m_updateOutlineIndexTimer->start();
}

void CPPEditorWidget::highlightUses(const QList<SemanticInfo::Use> &uses,
                                    QList<QTextEdit::ExtraSelection> *selections)
{
    bool isUnused = false;

    if (uses.size() == 1)
        isUnused = true;

    foreach (const SemanticInfo::Use &use, uses) {
        if (use.isInvalid())
            continue;

        QTextEdit::ExtraSelection sel;
        if (isUnused)
            sel.format = m_occurrencesUnusedFormat;
        else
            sel.format = m_occurrencesFormat;

        const int anchor = document()->findBlockByNumber(use.line - 1).position() + use.column - 1;
        const int position = anchor + use.length;

        sel.cursor = QTextCursor(document());
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        selections->append(sel);
    }
}

void CPPEditorWidget::updateOutlineIndexNow()
{
    if (!m_outlineModel->document())
        return;

    if (m_outlineModel->document()->editorRevision() != editorRevision()) {
        m_updateOutlineIndexTimer->start();
        return;
    }

    m_updateOutlineIndexTimer->stop();

    m_outlineModelIndex = QModelIndex(); //invalidate
    QModelIndex comboIndex = outlineModelIndex();

    if (comboIndex.isValid()) {
        bool blocked = m_outlineCombo->blockSignals(true);

        // There is no direct way to select a non-root item
        m_outlineCombo->setRootModelIndex(m_proxyModel->mapFromSource(comboIndex.parent()));
        m_outlineCombo->setCurrentIndex(m_proxyModel->mapFromSource(comboIndex).row());
        m_outlineCombo->setRootModelIndex(QModelIndex());

        updateOutlineToolTip();

        m_outlineCombo->blockSignals(blocked);
    }
}

void CPPEditorWidget::updateOutlineToolTip()
{
    m_outlineCombo->setToolTip(m_outlineCombo->currentText());
}

void CPPEditorWidget::updateUses()
{
    if (editorRevision() != m_highlightRevision)
        m_highlighter.cancel();

    // Block premature semantic info calculation when editor is created.
    if (m_modelManager->cppEditorSupport(editor())->initialized())
        m_updateUsesTimer->start();
}

void CPPEditorWidget::updateUsesNow()
{
    if (m_currentRenameSelection != NoCurrentRenameSelection)
        return;

    semanticRehighlight();
}

void CPPEditorWidget::highlightSymbolUsages(int from, int to)
{
    if (editorRevision() != m_highlightRevision)
        return; // outdated

    else if (m_highlighter.isCanceled())
        return; // aborted

    TextEditor::SyntaxHighlighter *highlighter = baseTextDocument()->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);

    TextEditor::SemanticHighlighter::incrementalApplyExtraAdditionalFormats(
                highlighter, m_highlighter, from, to, m_semanticHighlightFormatMap);
}

void CPPEditorWidget::finishHighlightSymbolUsages()
{
    if (editorRevision() != m_highlightRevision)
        return; // outdated

    if (m_highlighter.isCanceled())
        return; // aborted

    TextEditor::SyntaxHighlighter *highlighter = baseTextDocument()->syntaxHighlighter();
    QTC_ASSERT(highlighter, return);

    TextEditor::SemanticHighlighter::clearExtraAdditionalFormatsUntilEnd(
                highlighter, m_highlighter);
}

void CPPEditorWidget::switchDeclarationDefinition(bool inNextSplit)
{
    if (!m_modelManager)
        return;

    if (!m_lastSemanticInfo.doc)
        return;

    // Find function declaration or definition under cursor
    Function *functionDefinitionSymbol = 0;
    Symbol *functionDeclarationSymbol = 0;

    ASTPath astPathFinder(m_lastSemanticInfo.doc);
    const QList<AST *> astPath = astPathFinder(textCursor());

    for (int i = 0, size = astPath.size(); i < size; ++i) {
        AST *ast = astPath.at(i);
        if (FunctionDefinitionAST *functionDefinitionAST = ast->asFunctionDefinition()) {
            if ((functionDefinitionSymbol = functionDefinitionAST->symbol))
                break; // Function definition found!
        } else if (SimpleDeclarationAST *simpleDeclaration = ast->asSimpleDeclaration()) {
            if (List<Symbol *> *symbols = simpleDeclaration->symbols) {
                if (Symbol *symbol = symbols->value) {
                    if (symbol->isDeclaration() && symbol->type()->isFunctionType()) {
                        functionDeclarationSymbol = symbol;
                        break; // Function declaration found!
                    }
                }
            }
        }
    }

    // Link to function definition/declaration
    CPPEditorWidget::Link symbolLink;
    if (functionDeclarationSymbol) {
        symbolLink = linkToSymbol(symbolFinder()
            ->findMatchingDefinition(functionDeclarationSymbol, modelManager()->snapshot()));
    } else if (functionDefinitionSymbol) {
        const Snapshot snapshot = m_modelManager->snapshot();
        LookupContext context(m_lastSemanticInfo.doc, snapshot);
        ClassOrNamespace *binding = context.lookupType(functionDefinitionSymbol);
        const QList<LookupItem> declarations = context.lookup(functionDefinitionSymbol->name(),
            functionDefinitionSymbol->enclosingScope());

        QList<Symbol *> best;
        foreach (const LookupItem &r, declarations) {
            if (Symbol *decl = r.declaration()) {
                if (Function *funTy = decl->type()->asFunctionType()) {
                    if (funTy->isEqualTo(functionDefinitionSymbol)) {
                        if (decl != functionDefinitionSymbol && binding == r.binding())
                            best.prepend(decl);
                        else
                            best.append(decl);
                    }
                }
            }
        }

        if (best.isEmpty())
            return;
        symbolLink = linkToSymbol(best.first());
    }

    // Open Editor at link position
    if (symbolLink.hasValidTarget())
        openCppEditorAt(symbolLink, inNextSplit != alwaysOpenLinksInNextSplit());
}

static inline LookupItem skipForwardDeclarations(const QList<LookupItem> &resolvedSymbols)
{
    QList<LookupItem> candidates = resolvedSymbols;

    LookupItem result = candidates.first();
    const FullySpecifiedType ty = result.type().simplified();

    if (ty->isForwardClassDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->isObjCForwardClassDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isObjCForwardClassDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    if (ty->isObjCForwardProtocolDeclarationType()) {
        while (! candidates.isEmpty()) {
            LookupItem r = candidates.takeFirst();

            if (! r.type()->isObjCForwardProtocolDeclarationType()) {
                result = r;
                break;
            }
        }
    }

    return result;
}

CPPEditorWidget::Link CPPEditorWidget::attemptFuncDeclDef(const QTextCursor &cursor,
                                                          const Document::Ptr &doc,
                                                          Snapshot snapshot) const
{
    snapshot.insert(doc);

    Link result;

    QList<AST *> path = ASTPath(doc)(cursor);

    if (path.size() < 5)
        return result;

    NameAST *name = path.last()->asName();
    if (!name)
        return result;

    if (QualifiedNameAST *qName = path.at(path.size() - 2)->asQualifiedName()) {
        // TODO: check which part of the qualified name we're on
        if (qName->unqualified_name != name)
            return result;
    }

    for (int i = path.size() - 1; i != -1; --i) {
        AST *node = path.at(i);

        if (node->asParameterDeclaration() != 0)
            return result;
    }

    AST *declParent = 0;
    DeclaratorAST *decl = 0;
    for (int i = path.size() - 2; i > 0; --i) {
        if ((decl = path.at(i)->asDeclarator()) != 0) {
            declParent = path.at(i - 1);
            break;
        }
    }
    if (!decl || !declParent)
        return result;
    if (!decl->postfix_declarator_list || !decl->postfix_declarator_list->value)
        return result;
    FunctionDeclaratorAST *funcDecl = decl->postfix_declarator_list->value->asFunctionDeclarator();
    if (!funcDecl)
        return result;

    Symbol *target = 0;
    if (FunctionDefinitionAST *funDef = declParent->asFunctionDefinition()) {
        QList<Declaration *> candidates =
                symbolFinder()->findMatchingDeclaration(LookupContext(doc, snapshot),
                                                        funDef->symbol);
        if (!candidates.isEmpty()) // TODO: improve disambiguation
            target = candidates.first();
    } else if (declParent->asSimpleDeclaration()) {
        target = symbolFinder()->findMatchingDefinition(funcDecl->symbol, snapshot);
    }

    if (target) {
        result = linkToSymbol(target);

        unsigned startLine, startColumn, endLine, endColumn;
        doc->translationUnit()->getTokenStartPosition(name->firstToken(), &startLine, &startColumn);
        doc->translationUnit()->getTokenEndPosition(name->lastToken() - 1, &endLine, &endColumn);

        QTextDocument *textDocument = cursor.document();
        result.linkTextStart =
                textDocument->findBlockByNumber(startLine - 1).position() + startColumn - 1;
        result.linkTextEnd =
                textDocument->findBlockByNumber(endLine - 1).position() + endColumn - 1;
    }

    return result;
}

CPPEditorWidget::Link CPPEditorWidget::findMacroLink(const QByteArray &name) const
{
    if (! name.isEmpty()) {
        if (Document::Ptr doc = m_lastSemanticInfo.doc) {
            const Snapshot snapshot = m_modelManager->snapshot();
            QSet<QString> processed;
            return findMacroLink(name, doc, snapshot, &processed);
        }
    }

    return Link();
}

CPPEditorWidget::Link CPPEditorWidget::findMacroLink(const QByteArray &name,
                                                     Document::Ptr doc,
                                                     const Snapshot &snapshot,
                                                     QSet<QString> *processed) const
{
    if (doc && ! name.startsWith('<') && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        foreach (const Macro &macro, doc->definedMacros()) {
            if (macro.name() == name) {
                Link link;
                link.targetFileName = macro.fileName();
                link.targetLine = macro.line();
                return link;
            }
        }

        const QList<Document::Include> includes = doc->includes();
        for (int index = includes.size() - 1; index != -1; --index) {
            const Document::Include &i = includes.at(index);
            Link link = findMacroLink(name, snapshot.document(i.resolvedFileName()), snapshot,
                                      processed);
            if (link.hasValidTarget())
                return link;
        }
    }

    return Link();
}

QString CPPEditorWidget::identifierUnderCursor(QTextCursor *macroCursor) const
{
    macroCursor->movePosition(QTextCursor::StartOfWord);
    macroCursor->movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return macroCursor->selectedText();
}

CPPEditorWidget::Link CPPEditorWidget::findLinkAt(const QTextCursor &cursor, bool resolveTarget)
{
    Link link;

    if (!m_modelManager)
        return link;

    // Move to end of identifier
    QTextCursor tc = cursor;
    QChar ch = document()->characterAt(tc.position());
    while (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
        tc.movePosition(QTextCursor::NextCharacter);
        ch = document()->characterAt(tc.position());
    }

    const Snapshot &snapshot = m_modelManager->snapshot();

    // Try to macth decl/def. For this we need the semantic doc with the AST.
    if (m_lastSemanticInfo.doc
            && m_lastSemanticInfo.doc->translationUnit()
            && m_lastSemanticInfo.doc->translationUnit()->ast()) {
        int pos = tc.position();
        while (document()->characterAt(pos).isSpace())
            ++pos;
        if (document()->characterAt(pos) == QLatin1Char('(')) {
            link = attemptFuncDeclDef(cursor, m_lastSemanticInfo.doc, snapshot);
            if (link.hasValidLinkText())
                return link;
        }
    }

    int lineNumber = 0, positionInBlock = 0;
    convertPosition(cursor.position(), &lineNumber, &positionInBlock);
    const unsigned line = lineNumber;
    const unsigned column = positionInBlock + 1;

    // Try to find a signal or slot inside SIGNAL() or SLOT()
    int beginOfToken = 0;
    int endOfToken = 0;

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(true);
    const QString blockText = cursor.block().text();
    const QList<Token> tokens = tokenize(blockText,
                                         BackwardsScanner::previousBlockState(cursor.block()));

    bool recognizedQtMethod = false;

    for (int i = 0; i < tokens.size(); ++i) {
        const Token &tk = tokens.at(i);

        if (((unsigned) positionInBlock) >= tk.begin() && ((unsigned) positionInBlock) <= tk.end()) {
            if (i >= 2 && tokens.at(i).is(T_IDENTIFIER) && tokens.at(i - 1).is(T_LPAREN)
                && (tokens.at(i - 2).is(T_SIGNAL) || tokens.at(i - 2).is(T_SLOT))) {

                // token[i] == T_IDENTIFIER
                // token[i + 1] == T_LPAREN
                // token[.....] == ....
                // token[i + n] == T_RPAREN

                if (i + 1 < tokens.size() && tokens.at(i + 1).is(T_LPAREN)) {
                    // skip matched parenthesis
                    int j = i - 1;
                    int depth = 0;

                    for (; j < tokens.size(); ++j) {
                        if (tokens.at(j).is(T_LPAREN))
                            ++depth;

                        else if (tokens.at(j).is(T_RPAREN)) {
                            if (! --depth)
                                break;
                        }
                    }

                    if (j < tokens.size()) {
                        QTextBlock block = cursor.block();

                        beginOfToken = block.position() + tokens.at(i).begin();
                        endOfToken = block.position() + tokens.at(i).end();

                        tc.setPosition(block.position() + tokens.at(j).end());
                        recognizedQtMethod = true;
                    }
                }
            }
            break;
        }
    }

    // Now we prefer the doc from the snapshot with macros expanded.
    Document::Ptr doc = snapshot.document(editorDocument()->fileName());
    if (!doc) {
        doc = m_lastSemanticInfo.doc;
        if (!doc)
            return link;
    }

    if (!recognizedQtMethod) {
        const QTextBlock block = tc.block();
        int pos = cursor.positionInBlock();
        QChar ch = document()->characterAt(cursor.position());
        if (pos > 0 && ! (ch.isLetterOrNumber() || ch == QLatin1Char('_')))
            --pos; // positionInBlock points to a delimiter character.
        const Token tk = SimpleLexer::tokenAt(block.text(), pos,
                                              BackwardsScanner::previousBlockState(block), true);

        beginOfToken = block.position() + tk.begin();
        endOfToken = block.position() + tk.end();

        // Handle include directives
        if (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)) {
            const unsigned lineno = cursor.blockNumber() + 1;
            foreach (const Document::Include &incl, doc->includes()) {
                if (incl.line() == lineno) {
                    link.targetFileName = incl.resolvedFileName();
                    link.linkTextStart = beginOfToken + 1;
                    link.linkTextEnd = endOfToken - 1;
                    return link;
                }
            }
        }

        if (tk.isNot(T_IDENTIFIER) && tk.kind() < T_FIRST_QT_KEYWORD && tk.kind() > T_LAST_KEYWORD)
            return link;

        tc.setPosition(endOfToken);
    }

    // Handle macro uses
    const Macro *macro = doc->findMacroDefinitionAt(line);
    if (macro) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = identifierUnderCursor(&macroCursor).toLatin1();
        if (macro->name() == name)
            return link;    //already on definition!
    } else {
        const Document::MacroUse *use = doc->findMacroUseAt(endOfToken - 1);
        if (use && use->macro().fileName() != CppModelManagerInterface::configurationFileName()) {
            const Macro &macro = use->macro();
            link.targetFileName = macro.fileName();
            link.targetLine = macro.line();
            link.linkTextStart = use->begin();
            link.linkTextEnd = use->end();
            return link;
        }
    }

    // Find the last symbol up to the cursor position
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return link;

    // Evaluate the type of the expression under the cursor
    ExpressionUnderCursor expressionUnderCursor;
    QString expression = expressionUnderCursor(tc);

    for (int pos = tc.position();; ++pos) {
        const QChar ch = document()->characterAt(pos);
        if (ch.isSpace())
            continue;
        else {
            if (ch == QLatin1Char('(') && ! expression.isEmpty()) {
                tc.setPosition(pos);
                if (TextEditor::TextBlockUserData::findNextClosingParenthesis(&tc, true))
                    expression.append(tc.selectedText());
            }

            break;
        }
    }

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> resolvedSymbols =
            typeOfExpression.reference(expression.toUtf8(), scope, TypeOfExpression::Preprocess);

    if (!resolvedSymbols.isEmpty()) {
        LookupItem result = skipForwardDeclarations(resolvedSymbols);

        foreach (const LookupItem &r, resolvedSymbols) {
            if (Symbol *d = r.declaration()) {
                if (d->isDeclaration() || d->isFunction()) {
                    if (editorDocument()->fileName() == QString::fromUtf8(d->fileName(), d->fileNameLength())) {
                        if (unsigned(lineNumber) == d->line() && unsigned(positionInBlock) >= d->column()) { // ### TODO: check the end
                            result = r; // take the symbol under cursor.
                            break;
                        }
                    }
                } else if (d->isUsingDeclaration()) {
                    int tokenBeginLineNumber = 0, tokenBeginColumnNumber = 0;
                    convertPosition(beginOfToken, &tokenBeginLineNumber, &tokenBeginColumnNumber);
                    if (unsigned(tokenBeginLineNumber) > d->line()
                            || (unsigned(tokenBeginLineNumber) == d->line()
                                && unsigned(tokenBeginColumnNumber) > d->column())) {
                        result = r; // take the symbol under cursor.
                        break;
                    }
                }
            }
        }

        if (Symbol *symbol = result.declaration()) {
            Symbol *def = 0;

            if (resolveTarget) {
                Symbol *lastVisibleSymbol = doc->lastVisibleSymbolAt(line, column);

                def = findDefinition(symbol, snapshot);

                if (def == lastVisibleSymbol)
                    def = 0; // jump to declaration then.

                if (symbol->isForwardClassDeclaration())
                    def = symbolFinder()->findMatchingClassDeclaration(symbol, snapshot);
            }

            link = linkToSymbol(def ? def : symbol);
            link.linkTextStart = beginOfToken;
            link.linkTextEnd = endOfToken;
            return link;
        }
    }

    // Handle macro uses
    QTextCursor macroCursor = cursor;
    const QByteArray name = identifierUnderCursor(&macroCursor).toLatin1();
    link = findMacroLink(name);
    if (link.hasValidTarget()) {
        link.linkTextStart = macroCursor.selectionStart();
        link.linkTextEnd = macroCursor.selectionEnd();
        return link;
    }

    return Link();
}

Symbol *CPPEditorWidget::findDefinition(Symbol *symbol, const Snapshot &snapshot) const
{
    if (symbol->isFunction())
        return 0; // symbol is a function definition.

    else if (! symbol->type()->isFunctionType())
        return 0; // not a function declaration

    return symbolFinder()->findMatchingDefinition(symbol, snapshot);
}

unsigned CPPEditorWidget::editorRevision() const
{
    return document()->revision();
}

bool CPPEditorWidget::isOutdated() const
{
    if (m_lastSemanticInfo.revision != editorRevision())
        return true;

    return false;
}

SemanticInfo CPPEditorWidget::semanticInfo() const
{
    return m_lastSemanticInfo;
}

CPlusPlus::OverviewModel *CPPEditorWidget::outlineModel() const
{
    return m_outlineModel;
}

QModelIndex CPPEditorWidget::outlineModelIndex()
{
    if (!m_outlineModelIndex.isValid()) {
        int line = 0, column = 0;
        convertPosition(position(), &line, &column);
        m_outlineModelIndex = indexForPosition(line, column);
        emit outlineModelIndexChanged(m_outlineModelIndex);
    }

    return m_outlineModelIndex;
}

bool CPPEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        // handle escape manually if a rename is active
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape
                && m_currentRenameSelection != NoCurrentRenameSelection) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return BaseTextEditorWidget::event(e);
}

void CPPEditorWidget::performQuickFix(int index)
{
    TextEditor::QuickFixOperation::Ptr op = m_quickFixes.at(index);
    op->perform();
}

void CPPEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    QPointer<QMenu> menu(new QMenu(this));

    Core::ActionContainer *mcontext = Core::ActionManager::actionContainer(Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    QMenu *quickFixMenu = new QMenu(tr("&Refactor"), menu);
    quickFixMenu->addAction(Core::ActionManager::command(
                                Constants::RENAME_SYMBOL_UNDER_CURSOR)->action());

    QSignalMapper mapper;
    connect(&mapper, SIGNAL(mapped(int)), this, SLOT(performQuickFix(int)));
    if (! isOutdated()) {
        TextEditor::IAssistInterface *interface =
            createAssistInterface(TextEditor::QuickFix, TextEditor::ExplicitlyInvoked);
        if (interface) {
            QScopedPointer<TextEditor::IAssistProcessor> processor(
                        CppEditorPlugin::instance()->quickFixProvider()->createProcessor());
            QScopedPointer<TextEditor::IAssistProposal> proposal(processor->perform(interface));
            if (!proposal.isNull()) {
                TextEditor::BasicProposalItemListModel *model =
                        static_cast<TextEditor::BasicProposalItemListModel *>(proposal->model());
                for (int index = 0; index < model->size(); ++index) {
                    TextEditor::BasicProposalItem *item =
                            static_cast<TextEditor::BasicProposalItem *>(model->proposalItem(index));
                    TextEditor::QuickFixOperation::Ptr op =
                            item->data().value<TextEditor::QuickFixOperation::Ptr>();
                    m_quickFixes.append(op);
                    QAction *action = quickFixMenu->addAction(op->description());
                    mapper.setMapping(action, index);
                    connect(action, SIGNAL(triggered()), &mapper, SLOT(map()));
                }
                delete model;
            }
        }
    }

    foreach (QAction *action, contextMenu->actions()) {
        menu->addAction(action);
        if (action->objectName() == QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT))
            menu->addMenu(quickFixMenu);
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (!menu)
        return;
    m_quickFixes.clear();
    delete menu;
}

void CPPEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (m_currentRenameSelection == NoCurrentRenameSelection) {
        if (!handleDocumentationComment(e))
            TextEditor::BaseTextEditorWidget::keyPressEvent(e);
        return;
    }

    // key handling for renames

    QTextCursor cursor = textCursor();
    const QTextCursor::MoveMode moveMode = (e->modifiers() & Qt::ShiftModifier)
            ? QTextCursor::KeepAnchor
            : QTextCursor::MoveAnchor;

    switch (e->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        abortRename();
        e->accept();
        return;
    case Qt::Key_Home: {
        // Send home to start of name when within the name and not at the start
        if (cursor.position() > m_currentRenameSelectionBegin.position()
               && cursor.position() <= m_currentRenameSelectionEnd.position()) {
            cursor.setPosition(m_currentRenameSelectionBegin.position(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_End: {
        // Send end to end of name when within the name and not at the end
        if (cursor.position() >= m_currentRenameSelectionBegin.position()
               && cursor.position() < m_currentRenameSelectionEnd.position()) {
            cursor.setPosition(m_currentRenameSelectionEnd.position(), moveMode);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Backspace: {
        if (cursor.position() == m_currentRenameSelectionBegin.position()
            && !cursor.hasSelection()) {
            // Eat backspace at start of name when there is no selection
            e->accept();
            return;
        }
        break;
    }
    case Qt::Key_Delete: {
        if (cursor.position() == m_currentRenameSelectionEnd.position()
            && !cursor.hasSelection()) {
            // Eat delete at end of name when there is no selection
            e->accept();
            return;
        }
        break;
    }
    default: {
        break;
    }
    } // switch

    startRename();

    bool wantEditBlock = (cursor.position() >= m_currentRenameSelectionBegin.position()
                          && cursor.position() <= m_currentRenameSelectionEnd.position());

    if (wantEditBlock) {
        // possible change inside rename selection
        if (m_firstRenameChange)
            cursor.beginEditBlock();
        else
            cursor.joinPreviousEditBlock();
        m_firstRenameChange = false;
    }
    TextEditor::BaseTextEditorWidget::keyPressEvent(e);
    if (wantEditBlock)
        cursor.endEditBlock();
    finishRename();
}

Core::IEditor *CPPEditor::duplicate(QWidget *parent)
{
    CPPEditorWidget *newEditor = new CPPEditorWidget(parent);
    newEditor->duplicateFrom(editorWidget());
    // A new QTextDocument was set, so update our signal/slot connection to the new document
    newEditor->updateContentsChangedSignal();
    CppEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editor();
}

Core::Id CPPEditor::id() const
{
    return Core::Id(CppEditor::Constants::CPPEDITOR_ID);
}

bool CPPEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (!TextEditor::BaseTextEditor::open(errorString, fileName, realFileName))
        return false;
    editorWidget()->setMimeType(
                Core::ICore::mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return true;
}

const Utils::CommentDefinition *CPPEditor::commentDefinition() const
{
    return &m_commentDefinition;
}

void CPPEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    CppHighlighter *highlighter
            = qobject_cast<CppHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(highlighterFormatCategories());
    highlighter->setFormats(formats.constBegin(), formats.constEnd());

    m_occurrencesFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES);
    m_occurrencesUnusedFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES_UNUSED);
    m_occurrencesUnusedFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_occurrencesUnusedFormat.setUnderlineColor(m_occurrencesUnusedFormat.foreground().color());
    m_occurrencesUnusedFormat.clearForeground();
    m_occurrencesUnusedFormat.setToolTip(tr("Unused variable"));
    m_occurrenceRenameFormat = fs.toTextCharFormat(TextEditor::C_OCCURRENCES_RENAME);

    m_semanticHighlightFormatMap[CppHighlightingSupport::TypeUse] =
            fs.toTextCharFormat(TextEditor::C_TYPE);
    m_semanticHighlightFormatMap[CppHighlightingSupport::LocalUse] =
            fs.toTextCharFormat(TextEditor::C_LOCAL);
    m_semanticHighlightFormatMap[CppHighlightingSupport::FieldUse] =
            fs.toTextCharFormat(TextEditor::C_FIELD);
    m_semanticHighlightFormatMap[CppHighlightingSupport::EnumerationUse] =
            fs.toTextCharFormat(TextEditor::C_ENUMERATION);
    m_semanticHighlightFormatMap[CppHighlightingSupport::VirtualMethodUse] =
            fs.toTextCharFormat(TextEditor::C_VIRTUAL_METHOD);
    m_semanticHighlightFormatMap[CppHighlightingSupport::LabelUse] =
            fs.toTextCharFormat(TextEditor::C_LABEL);
    m_semanticHighlightFormatMap[CppHighlightingSupport::MacroUse] =
            fs.toTextCharFormat(TextEditor::C_PREPROCESSOR);
    m_semanticHighlightFormatMap[CppHighlightingSupport::FunctionUse] =
            fs.toTextCharFormat(TextEditor::C_FUNCTION);
    m_semanticHighlightFormatMap[CppHighlightingSupport::PseudoKeywordUse] =
            fs.toTextCharFormat(TextEditor::C_KEYWORD);
    m_keywordFormat = fs.toTextCharFormat(TextEditor::C_KEYWORD);

    // only set the background, we do not want to modify foreground properties
    // set by the syntax highlighter or the link
    m_occurrencesFormat.clearForeground();
    m_occurrenceRenameFormat.clearForeground();

    // Clear all additional formats since they may have changed
    QTextBlock b = document()->firstBlock();
    while (b.isValid()) {
        QList<QTextLayout::FormatRange> noFormats;
        highlighter->setExtraAdditionalFormats(b, noFormats);
        b = b.next();
    }

    // This also triggers an update of the additional formats
    highlighter->rehighlight();
}

void CPPEditorWidget::setTabSettings(const TextEditor::TabSettings &ts)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());

    TextEditor::BaseTextEditorWidget::setTabSettings(ts);
}

void CPPEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

void CPPEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{
    CppTools::QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

CPPEditorWidget::Link CPPEditorWidget::linkToSymbol(CPlusPlus::Symbol *symbol)
{
    if (!symbol)
        return Link();

    const QString filename = QString::fromUtf8(symbol->fileName(),
                                               symbol->fileNameLength());

    unsigned line = symbol->line();
    unsigned column = symbol->column();

    if (column)
        --column;

    if (symbol->isGenerated())
        column = 0;

    return Link(filename, line, column);
}

bool CPPEditorWidget::openCppEditorAt(const Link &link, bool inNextSplit)
{
    if (!link.hasValidTarget())
        return false;

    Core::EditorManager::OpenEditorFlags flags;
    if (inNextSplit)
        flags |= Core::EditorManager::OpenInOtherSplit;
    return Core::EditorManager::openEditorAt(link.targetFileName,
                                             link.targetLine,
                                             link.targetColumn,
                                             Constants::CPPEDITOR_ID,
                                             flags);
}

void CPPEditorWidget::semanticRehighlight(bool force)
{
    m_modelManager->cppEditorSupport(editor())->recalculateSemanticInfoDetached(force);
}

void CPPEditorWidget::highlighterStarted(QFuture<TextEditor::HighlightingResult> *highlighter,
                                         unsigned revision)
{
    m_highlighter = *highlighter;
    m_highlightRevision = revision;
    m_highlightWatcher.setFuture(m_highlighter);
}

void CPPEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo)
{
    if (semanticInfo.revision != editorRevision()) {
        // got outdated semantic info
        semanticRehighlight();
        return;
    }

    m_lastSemanticInfo = semanticInfo; // update the semantic info

    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    QList<QTextEdit::ExtraSelection> unusedSelections;

    m_renameSelections.clear();
    m_currentRenameSelection = NoCurrentRenameSelection;

    // We can use the semanticInfo's snapshot (and avoid locking), but not its
    // document, since it doesn't contain expanded macros.
    LookupContext context(semanticInfo.snapshot.document(editorDocument()->fileName()),
                          semanticInfo.snapshot);

    SemanticInfo::LocalUseIterator it(semanticInfo.localUses);
    while (it.hasNext()) {
        it.next();
        const QList<SemanticInfo::Use> &uses = it.value();

        bool good = false;
        foreach (const SemanticInfo::Use &use, uses) {
            unsigned l = line;
            unsigned c = column + 1; // convertCursorPosition() returns a 0-based column number.
            if (l == use.line && c >= use.column && c <= (use.column + use.length)) {
                good = true;
                break;
            }
        }

        if (uses.size() == 1) {
            if (!CppTools::isOwnershipRAIIType(it.key(), context)) {
                // it's an unused declaration
                highlightUses(uses, &unusedSelections);
            }
        } else if (good && m_renameSelections.isEmpty()) {
            highlightUses(uses, &m_renameSelections);
        }
    }

    setExtraSelections(UnusedSymbolSelection, unusedSelections);

    if (! m_renameSelections.isEmpty())
        setExtraSelections(CodeSemanticsSelection, m_renameSelections); // ###
    else {
        markSymbols(textCursor(), semanticInfo);
    }

    m_lastSemanticInfo.forced = false; // clear the forced flag

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

QModelIndex CPPEditorWidget::indexForPosition(int line, int column,
                                              const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;

    const int rowCount = m_outlineModel->rowCount(rootIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_outlineModel->index(row, 0, rootIndex);
        Symbol *symbol = m_outlineModel->symbolFromIndex(index);
        if (symbol && symbol->line() > unsigned(line))
            break;
        lastIndex = index;
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(line, column, lastIndex);
    }

    return lastIndex;
}

QVector<TextEditor::TextStyle> CPPEditorWidget::highlighterFormatCategories()
{
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
                   << TextEditor::C_VISUAL_WHITESPACE;
    }
    return categories;
}

TextEditor::IAssistInterface *CPPEditorWidget::createAssistInterface(
    TextEditor::AssistKind kind,
    TextEditor::AssistReason reason) const
{
    if (kind == TextEditor::Completion) {
        if (m_completionSupport)
            return m_completionSupport->createAssistInterface(
                        ProjectExplorer::ProjectExplorerPlugin::currentProject(),
                        document(), position(), reason);
    } else if (kind == TextEditor::QuickFix) {
        if (!semanticInfo().doc || isOutdated())
            return 0;
        return new CppQuickFixAssistInterface(const_cast<CPPEditorWidget *>(this), reason);
    }
    return 0;
}

QSharedPointer<FunctionDeclDefLink> CPPEditorWidget::declDefLink() const
{
    return m_declDefLink;
}

void CPPEditorWidget::onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker)
{
    if (marker.data.canConvert<FunctionDeclDefLink::Marker>())
        applyDeclDefLinkChanges(true);
}

void CPPEditorWidget::updateFunctionDeclDefLink()
{
    const int pos = textCursor().selectionStart();

    // if there's already a link, abort it if the cursor is outside or the name changed
    // (adding a prefix is an exception since the user might type a return type)
    if (m_declDefLink
            && (pos < m_declDefLink->linkSelection.selectionStart()
                || pos > m_declDefLink->linkSelection.selectionEnd()
                || !m_declDefLink->nameSelection.selectedText().trimmed()
                    .endsWith(m_declDefLink->nameInitial))) {
        abortDeclDefLink();
        return;
    }

    // don't start a new scan if there's one active and the cursor is already in the scanned area
    const QTextCursor scannedSelection = m_declDefLinkFinder->scannedSelection();
    if (!scannedSelection.isNull()
            && scannedSelection.selectionStart() <= pos
            && scannedSelection.selectionEnd() >= pos) {
        return;
    }

    m_updateFunctionDeclDefLinkTimer->start();
}

void CPPEditorWidget::updateFunctionDeclDefLinkNow()
{
    if (Core::EditorManager::currentEditor() != editor())
        return;
    if (m_declDefLink) {
        // update the change marker
        const Utils::ChangeSet changes = m_declDefLink->changes(m_lastSemanticInfo.snapshot);
        if (changes.isEmpty())
            m_declDefLink->hideMarker(this);
        else
            m_declDefLink->showMarker(this);
        return;
    }
    if (!m_lastSemanticInfo.doc || isOutdated())
        return;

    Snapshot snapshot = CppModelManagerInterface::instance()->snapshot();
    snapshot.insert(m_lastSemanticInfo.doc);

    m_declDefLinkFinder->startFindLinkAt(textCursor(), m_lastSemanticInfo.doc, snapshot);
}

void CPPEditorWidget::onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    m_declDefLink = link;

    // disable the link if content of the target editor changes
    TextEditor::BaseTextEditorWidget *targetEditor =
            TextEditor::RefactoringChanges::editorForFile(link->targetFile->fileName());
    if (targetEditor && targetEditor != this) {
        connect(targetEditor, SIGNAL(textChanged()),
                this, SLOT(abortDeclDefLink()));
    }
}

void CPPEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!m_declDefLink)
        return;
    m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

void CPPEditorWidget::updateContentsChangedSignal()
{
    connect(document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(onContentsChanged(int,int,int)));
}

void CPPEditorWidget::abortDeclDefLink()
{
    if (!m_declDefLink)
        return;

    // undo connect from onFunctionDeclDefLinkFound
    TextEditor::BaseTextEditorWidget *targetEditor =
            TextEditor::RefactoringChanges::editorForFile(m_declDefLink->targetFile->fileName());
    if (targetEditor && targetEditor != this) {
        disconnect(targetEditor, SIGNAL(textChanged()),
                   this, SLOT(abortDeclDefLink()));
    }

    m_declDefLink->hideMarker(this);
    m_declDefLink.clear();
}

bool CPPEditorWidget::handleDocumentationComment(QKeyEvent *e)
{
    if (!m_commentsSettings.m_enableDoxygen
            && !m_commentsSettings.m_leadingAsterisks) {
        return false;
    }

    if (e->key() == Qt::Key_Return
            || e->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        if (!autoCompleter()->isInComment(cursor))
            return false;

        // We are interested on two particular cases:
        //   1) The cursor is right after a /**, /*!, /// or ///! and the user pressed enter.
        //      If Doxygen is enabled we need to generate an entire comment block.
        //   2) The cursor is already in the middle of a multi-line comment and the user pressed
        //      enter. If leading asterisk(s) is set we need to write a comment continuation
        //      with those.

        if (m_commentsSettings.m_enableDoxygen
                && cursor.positionInBlock() >= 3) {

            const int pos = cursor.position();

            if (isStartOfDoxygenComment(cursor)) {
                CppTools::DoxygenGenerator::DocumentationStyle style
                        = doxygenStyle(cursor, document());

                // Check if we're already in a CppStyle Doxygen comment => continuation
                // Needs special handling since CppStyle does not have start and end markers
                if ((style == CppTools::DoxygenGenerator::CppStyleA
                     || style == CppTools::DoxygenGenerator::CppStyleB)
                        && isCppStyleContinuation(cursor)) {
                    return handleDoxygenCppStyleContinuation(cursor, e);
                }

                CppTools::DoxygenGenerator doxygen;
                doxygen.setStyle(style);
                doxygen.setAddLeadingAsterisks(m_commentsSettings.m_leadingAsterisks);
                doxygen.setGenerateBrief(m_commentsSettings.m_generateBrief);
                doxygen.setStartComment(false);

                // Move until we reach any possibly meaningful content.
                while (document()->characterAt(cursor.position()).isSpace()
                       && cursor.movePosition(QTextCursor::NextCharacter)) {
                }

                if (!cursor.atEnd()) {
                    const QString &comment = doxygen.generate(cursor);
                    if (!comment.isEmpty()) {
                        cursor.beginEditBlock();
                        cursor.setPosition(pos);
                        cursor.insertText(comment);
                        cursor.setPosition(pos - 3, QTextCursor::KeepAnchor);
                        indent(document(), cursor, QChar::Null);
                        cursor.endEditBlock();
                        e->accept();
                        return true;
                    }
                }

            }
        } // right after first doxygen comment

        return handleDoxygenContinuation(cursor,
                                         e,
                                         document(),
                                         m_commentsSettings.m_enableDoxygen,
                                         m_commentsSettings.m_leadingAsterisks);
    }

    return false;
}

bool CPPEditorWidget::isStartOfDoxygenComment(const QTextCursor &cursor) const
{
    const int pos = cursor.position();
    QString comment = QString(document()->characterAt(pos - 3))
            + document()->characterAt(pos - 2)
            + document()->characterAt(pos - 1);

    if ((comment == QLatin1String("/**"))
            || (comment == QLatin1String("/*!"))
            || (comment == QLatin1String("///"))
            || (comment == QLatin1String("//!"))) {
        return true;
    }
    return false;
}

void CPPEditorWidget::onCommentsSettingsChanged(const CppTools::CommentsSettings &settings)
{
    m_commentsSettings = settings;
}

#include <cppeditor.moc>
