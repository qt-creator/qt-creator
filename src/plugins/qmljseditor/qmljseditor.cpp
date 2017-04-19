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

#include "qmljseditor.h"

#include "qmljsautocompleter.h"
#include "qmljscompletionassist.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"
#include "qmljseditorplugin.h"
#include "qmljsfindreferences.h"
#include "qmljshighlighter.h"
#include "qmljshoverhandler.h"
#include "qmljsquickfixassist.h"
#include "qmloutlinemodel.h"

#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsevaluate.h>
#include <qmljs/qmljsicontextpane.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsutils.h>

#include <qmljstools/qmljstoolsconstants.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/modemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/texteditoractionhandler.h>

#include <utils/annotateditemdelegate.h>
#include <utils/changeset.h>
#include <utils/qtcassert.h>
#include <utils/uncommentselection.h>

#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMetaMethod>
#include <QPointer>
#include <QScopedPointer>
#include <QTextCodec>
#include <QTimer>
#include <QTreeView>

enum {
    UPDATE_USES_DEFAULT_INTERVAL = 150,
    UPDATE_OUTLINE_INTERVAL = 500 // msecs after new semantic info has been arrived / cursor has moved
};

using namespace Core;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSTools;
using namespace TextEditor;

namespace QmlJSEditor {
namespace Internal {

//
// QmlJSEditorWidget
//

QmlJSEditorWidget::QmlJSEditorWidget()
{
    m_findReferences = new FindReferences(this);
    setLanguageSettingsId(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
}

void QmlJSEditorWidget::finalizeInitialization()
{
    m_qmlJsEditorDocument = static_cast<QmlJSEditorDocument *>(textDocument());

    m_updateUsesTimer.setInterval(UPDATE_USES_DEFAULT_INTERVAL);
    m_updateUsesTimer.setSingleShot(true);
    connect(&m_updateUsesTimer, &QTimer::timeout, this, &QmlJSEditorWidget::updateUses);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            &m_updateUsesTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    m_updateOutlineIndexTimer.setInterval(UPDATE_OUTLINE_INTERVAL);
    m_updateOutlineIndexTimer.setSingleShot(true);
    connect(&m_updateOutlineIndexTimer, &QTimer::timeout,
            this, &QmlJSEditorWidget::updateOutlineIndexNow);

    textDocument()->setCodec(QTextCodec::codecForName("UTF-8")); // qml files are defined to be utf-8

    m_modelManager = ModelManagerInterface::instance();
    m_contextPane = ExtensionSystem::PluginManager::getObject<IContextPane>();

    m_modelManager->activateScan();

    m_contextPaneTimer.setInterval(UPDATE_OUTLINE_INTERVAL);
    m_contextPaneTimer.setSingleShot(true);
    connect(&m_contextPaneTimer, &QTimer::timeout, this, &QmlJSEditorWidget::updateContextPane);
    if (m_contextPane) {
        connect(this, &QmlJSEditorWidget::cursorPositionChanged,
                &m_contextPaneTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
        connect(m_contextPane, &IContextPane::closed, this, &QmlJSEditorWidget::showTextMarker);
    }

    connect(this->document(), &QTextDocument::modificationChanged,
            this, &QmlJSEditorWidget::modificationChanged);

    connect(m_qmlJsEditorDocument, &QmlJSEditorDocument::updateCodeWarnings,
            this, &QmlJSEditorWidget::updateCodeWarnings);

    connect(m_qmlJsEditorDocument, &QmlJSEditorDocument::semanticInfoUpdated,
            this, &QmlJSEditorWidget::semanticInfoUpdated);

    setRequestMarkEnabled(true);
    createToolBar();
}

QModelIndex QmlJSEditorWidget::outlineModelIndex()
{
    if (!m_outlineModelIndex.isValid()) {
        m_outlineModelIndex = indexForPosition(position());
    }
    return m_outlineModelIndex;
}

static void appendExtraSelectionsForMessages(
        QList<QTextEdit::ExtraSelection> *selections,
        const QList<DiagnosticMessage> &messages,
        const QTextDocument *document)
{
    foreach (const DiagnosticMessage &d, messages) {
        const int line = d.loc.startLine;
        const int column = qMax(1U, d.loc.startColumn);

        QTextEdit::ExtraSelection sel;
        QTextCursor c(document->findBlockByNumber(line - 1));
        sel.cursor = c;

        sel.cursor.setPosition(c.position() + column - 1);

        if (d.loc.length == 0) {
            if (sel.cursor.atBlockEnd())
                sel.cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
            else
                sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        } else {
            sel.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, d.loc.length);
        }

        const auto fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

        if (d.isWarning())
            sel.format = fontSettings.toTextCharFormat(TextEditor::C_WARNING);
        else
            sel.format = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

        sel.format.setToolTip(d.message);

        selections->append(sel);
    }
}

void QmlJSEditorWidget::updateCodeWarnings(Document::Ptr doc)
{
    if (doc->ast()) {
        setExtraSelections(CodeWarningsSelection, QList<QTextEdit::ExtraSelection>());
    } else if (doc->language().isFullySupportedLanguage()) {
        // show parsing errors
        QList<QTextEdit::ExtraSelection> selections;
        appendExtraSelectionsForMessages(&selections, doc->diagnosticMessages(), document());
        setExtraSelections(CodeWarningsSelection, selections);
    } else {
        setExtraSelections(CodeWarningsSelection, QList<QTextEdit::ExtraSelection>());
    }
}

void QmlJSEditorWidget::modificationChanged(bool changed)
{
    if (!changed && m_modelManager)
        m_modelManager->fileChangedOnDisk(textDocument()->filePath().toString());
}

bool QmlJSEditorWidget::isOutlineCursorChangesBlocked()
{
    return hasFocus() || m_blockOutLineCursorChanges;
}

void QmlJSEditorWidget::jumpToOutlineElement(int /*index*/)
{
    QModelIndex index = m_outlineCombo->view()->currentIndex();
    AST::SourceLocation location = m_qmlJsEditorDocument->outlineModel()->sourceLocation(index);

    if (!location.isValid())
        return;

    EditorManager::cutForwardNavigationHistory();
    EditorManager::addCurrentPositionToNavigationHistory();

    QTextCursor cursor = textCursor();
    cursor.setPosition(location.offset);
    setTextCursor(cursor);

    setFocus();
}

void QmlJSEditorWidget::updateOutlineIndexNow()
{
    if (!m_qmlJsEditorDocument->outlineModel()->document())
        return;

    if (m_qmlJsEditorDocument->outlineModel()->document()->editorRevision() != document()->revision()) {
        m_updateOutlineIndexTimer.start();
        return;
    }

    m_outlineModelIndex = QModelIndex(); // invalidate
    QModelIndex comboIndex = outlineModelIndex();
    emit outlineModelIndexChanged(m_outlineModelIndex);

    if (comboIndex.isValid()) {
        bool blocked = m_outlineCombo->blockSignals(true);

        // There is no direct way to select a non-root item
        m_outlineCombo->setRootModelIndex(comboIndex.parent());
        m_outlineCombo->setCurrentIndex(comboIndex.row());
        m_outlineCombo->setRootModelIndex(QModelIndex());

        m_outlineCombo->blockSignals(blocked);
    }
}
} // namespace Internal
} // namespace QmlJSEditor

class QtQuickToolbarMarker {};
Q_DECLARE_METATYPE(QtQuickToolbarMarker)

namespace QmlJSEditor {
namespace Internal {

template <class T>
static QList<RefactorMarker> removeMarkersOfType(const QList<RefactorMarker> &markers)
{
    QList<RefactorMarker> result;
    foreach (const RefactorMarker &marker, markers) {
        if (!marker.data.canConvert<T>())
            result += marker;
    }
    return result;
}

void QmlJSEditorWidget::updateContextPane()
{
    const SemanticInfo info = m_qmlJsEditorDocument->semanticInfo();
    if (m_contextPane && document() && info.isValid()
            && document()->revision() == info.document->editorRevision())
    {
        Node *oldNode = info.declaringMemberNoProperties(m_oldCursorPosition);
        Node *newNode = info.declaringMemberNoProperties(position());
        if (oldNode != newNode && m_oldCursorPosition != -1)
            m_contextPane->apply(this, info.document, 0, newNode, false);

        if (m_contextPane->isAvailable(this, info.document, newNode) &&
            !m_contextPane->widget()->isVisible()) {
            QList<RefactorMarker> markers = removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers());
            if (UiObjectMember *m = newNode->uiObjectMemberCast()) {
                const int start = qualifiedTypeNameId(m)->identifierToken.begin();
                for (UiQualifiedId *q = qualifiedTypeNameId(m); q; q = q->next) {
                    if (! q->next) {
                        const int end = q->identifierToken.end();
                        if (position() >= start && position() <= end) {
                            RefactorMarker marker;
                            QTextCursor tc(document());
                            tc.setPosition(end);
                            marker.cursor = tc;
                            marker.tooltip = tr("Show Qt Quick ToolBar");
                            marker.data = QVariant::fromValue(QtQuickToolbarMarker());
                            markers.append(marker);
                        }
                    }
                }
            }
            setRefactorMarkers(markers);
        } else if (oldNode != newNode) {
            setRefactorMarkers(removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers()));
        }
        m_oldCursorPosition = position();

        setSelectedElements();
    }
}

void QmlJSEditorWidget::showTextMarker()
{
    m_oldCursorPosition = -1;
    updateContextPane();
}

void QmlJSEditorWidget::updateUses()
{
    if (m_qmlJsEditorDocument->isSemanticInfoOutdated()) // will be updated when info is updated
        return;

    QList<QTextEdit::ExtraSelection> selections;
    foreach (const AST::SourceLocation &loc,
             m_qmlJsEditorDocument->semanticInfo().idLocations.value(wordUnderCursor())) {
        if (! loc.isValid())
            continue;

        QTextEdit::ExtraSelection sel;
        sel.format = textDocument()->fontSettings().toTextCharFormat(C_OCCURRENCES);
        sel.cursor = textCursor();
        sel.cursor.setPosition(loc.begin());
        sel.cursor.setPosition(loc.end(), QTextCursor::KeepAnchor);
        selections.append(sel);
    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

class SelectedElement: protected Visitor
{
    unsigned m_cursorPositionStart;
    unsigned m_cursorPositionEnd;
    QList<UiObjectMember *> m_selectedMembers;

public:
    SelectedElement()
        : m_cursorPositionStart(0), m_cursorPositionEnd(0) {}

    QList<UiObjectMember *> operator()(const Document::Ptr &doc, unsigned startPosition, unsigned endPosition)
    {
        m_cursorPositionStart = startPosition;
        m_cursorPositionEnd = endPosition;
        m_selectedMembers.clear();
        Node::accept(doc->qmlProgram(), this);
        return m_selectedMembers;
    }

protected:

    bool isSelectable(UiObjectMember *member) const
    {
        UiQualifiedId *id = qualifiedTypeNameId(member);
        if (id) {
            const QStringRef &name = id->name;
            if (!name.isEmpty() && name.at(0).isUpper())
                return true;
        }

        return false;
    }

    inline bool isIdBinding(UiObjectMember *member) const
    {
        if (UiScriptBinding *script = cast<UiScriptBinding *>(member)) {
            if (! script->qualifiedId)
                return false;
            else if (script->qualifiedId->name.isEmpty())
                return false;
            else if (script->qualifiedId->next)
                return false;

            const QStringRef &propertyName = script->qualifiedId->name;

            if (propertyName == QLatin1String("id"))
                return true;
        }

        return false;
    }

    inline bool containsCursor(unsigned begin, unsigned end)
    {
        return m_cursorPositionStart >= begin && m_cursorPositionEnd <= end;
    }

    inline bool intersectsCursor(unsigned begin, unsigned end)
    {
        return (m_cursorPositionEnd >= begin && m_cursorPositionStart <= end);
    }

    inline bool isRangeSelected() const
    {
        return (m_cursorPositionStart != m_cursorPositionEnd);
    }

    void postVisit(Node *ast)
    {
        if (!isRangeSelected() && !m_selectedMembers.isEmpty())
            return; // nothing to do, we already have the results.

        if (UiObjectMember *member = ast->uiObjectMemberCast()) {
            unsigned begin = member->firstSourceLocation().begin();
            unsigned end = member->lastSourceLocation().end();

            if ((isRangeSelected() && intersectsCursor(begin, end))
            || (!isRangeSelected() && containsCursor(begin, end)))
            {
                if (initializerOfObject(member) && isSelectable(member)) {
                    m_selectedMembers << member;
                    // move start towards end; this facilitates multiselection so that root is usually ignored.
                    m_cursorPositionStart = qMin(end, m_cursorPositionEnd);
                }
            }
        }
    }
};

void QmlJSEditorWidget::setSelectedElements()
{
    static const QMetaMethod selectedChangedSignal =
            QMetaMethod::fromSignal(&QmlJSEditorWidget::selectedElementsChanged);
    if (!isSignalConnected(selectedChangedSignal))
        return;

    QTextCursor tc = textCursor();
    QString wordAtCursor;
    QList<UiObjectMember *> offsets;

    unsigned startPos;
    unsigned endPos;

    if (tc.hasSelection()) {
        startPos = tc.selectionStart();
        endPos = tc.selectionEnd();
    } else {
        tc.movePosition(QTextCursor::StartOfWord);
        tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        startPos = textCursor().position();
        endPos = textCursor().position();
    }

    if (m_qmlJsEditorDocument->semanticInfo().isValid()) {
        SelectedElement selectedMembers;
        QList<UiObjectMember *> members
                = selectedMembers(m_qmlJsEditorDocument->semanticInfo().document, startPos, endPos);
        if (!members.isEmpty()) {
            foreach (UiObjectMember *m, members) {
                offsets << m;
            }
        }
    }
    wordAtCursor = tc.selectedText();

    emit selectedElementsChanged(offsets, wordAtCursor);
}

void QmlJSEditorWidget::applyFontSettings()
{
    TextEditorWidget::applyFontSettings();
    if (!m_qmlJsEditorDocument->isSemanticInfoOutdated())
        updateUses();
}


QString QmlJSEditorWidget::wordUnderCursor() const
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

void QmlJSEditorWidget::createToolBar()
{
    m_outlineCombo = new QComboBox;
    m_outlineCombo->setMinimumContentsLength(22);
    m_outlineCombo->setModel(m_qmlJsEditorDocument->outlineModel());

    QTreeView *treeView = new QTreeView;

    Utils::AnnotatedItemDelegate *itemDelegate = new Utils::AnnotatedItemDelegate(this);
    itemDelegate->setDelimiter(QLatin1String(" "));
    itemDelegate->setAnnotationRole(QmlOutlineModel::AnnotationRole);
    treeView->setItemDelegateForColumn(0, itemDelegate);

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

    connect(m_outlineCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &QmlJSEditorWidget::jumpToOutlineElement);
    connect(m_qmlJsEditorDocument->outlineModel(), &QmlOutlineModel::updated,
            static_cast<QTreeView *>(m_outlineCombo->view()), &QTreeView::expandAll);

    connect(this, &QmlJSEditorWidget::cursorPositionChanged,
            &m_updateOutlineIndexTimer, static_cast<void (QTimer::*)()>(&QTimer::start));

    insertExtraToolBarWidget(TextEditorWidget::Left, m_outlineCombo);
}

class CodeModelInspector : public MemberProcessor
{
public:
    explicit CodeModelInspector(const CppComponentValue *processingValue, QTextStream *stream) :
        m_processingValue(processingValue),
        m_stream(stream),
        m_indent(QLatin1String("    "))
    {
    }

    bool processProperty(const QString &name, const Value *value,
                                 const PropertyInfo &propertyInfo) override
    {
        QString type;
        if (const CppComponentValue *cpp = value->asCppComponentValue())
            type = cpp->metaObject()->className();
        else
            type = m_processingValue->propertyType(name);

        if (propertyInfo.isList())
            type = QStringLiteral("list<%1>").arg(type);

        *m_stream << m_indent;
        if (!propertyInfo.isWriteable())
            *m_stream << "readonly ";
        *m_stream << "property " << type << " " << name << endl;

        return true;
    }
    bool processSignal(const QString &name, const Value *value) override
    {
        *m_stream << m_indent << "signal " << name << stringifyFunctionParameters(value) << endl;
        return true;
    }
    bool processSlot(const QString &name, const Value *value) override
    {
        *m_stream << m_indent << "function " << name << stringifyFunctionParameters(value) << endl;
        return true;
    }
    bool processGeneratedSlot(const QString &name, const Value *value) override
    {
        *m_stream << m_indent << "/*generated*/ function " << name
                  << stringifyFunctionParameters(value) << endl;
        return true;
    }

private:
    QString stringifyFunctionParameters(const Value *value) const
    {
        QStringList params;
        const QmlJS::MetaFunction *metaFunction = value->asMetaFunction();
        if (metaFunction) {
            QStringList paramNames = metaFunction->fakeMetaMethod().parameterNames();
            QStringList paramTypes = metaFunction->fakeMetaMethod().parameterTypes();
            for (int i = 0; i < paramTypes.size(); ++i) {
                QString typeAndNamePair = paramTypes.at(i);
                if (paramNames.size() > i) {
                    QString paramName = paramNames.at(i);
                    if (!paramName.isEmpty())
                        typeAndNamePair += QLatin1Char(' ') + paramName;
                }
                params.append(typeAndNamePair);
            }
        }
        return QLatin1Char('(') + params.join(QLatin1String(", ")) + QLatin1Char(')');
    }

private:
    const CppComponentValue *m_processingValue;
    QTextStream *m_stream;
    const QString m_indent;
};

static const CppComponentValue *findCppComponentToInspect(const SemanticInfo &semanticInfo,
                                                          const unsigned cursorPosition)
{
    AST::Node *node = semanticInfo.astNodeAt(cursorPosition);
    if (!node)
        return 0;

    const ScopeChain scopeChain = semanticInfo.scopeChain(semanticInfo.rangePath(cursorPosition));
    Evaluate evaluator(&scopeChain);
    const Value *value = evaluator.reference(node);
    if (!value)
        return 0;

    return value->asCppComponentValue();
}

static QString inspectCppComponent(const CppComponentValue *cppValue)
{
    QString result;
    QTextStream bufWriter(&result);

    // for QtObject
    QString superClassName = cppValue->metaObject()->superclassName();
    if (superClassName.isEmpty())
        superClassName = cppValue->metaObject()->className();

    bufWriter << "import QtQuick " << cppValue->importVersion().toString() << endl
              << "// " << cppValue->metaObject()->className()
              << " imported as " << cppValue->moduleName()  << " "
              << cppValue->importVersion().toString() << endl
              << endl
              << superClassName << " {" << endl;

    CodeModelInspector insp(cppValue, &bufWriter);
    cppValue->processMembers(&insp);

    bufWriter << endl;
    const int enumeratorCount = cppValue->metaObject()->enumeratorCount();
    for (int index = cppValue->metaObject()->enumeratorOffset(); index < enumeratorCount; ++index) {
        LanguageUtils::FakeMetaEnum enumerator = cppValue->metaObject()->enumerator(index);
        bufWriter << "    // Enum " << enumerator.name() << " { " <<
                     enumerator.keys().join(QLatin1Char(',')) << " }" << endl;
    }

    bufWriter << "}" << endl;
    return result;
}

void QmlJSEditorWidget::inspectElementUnderCursor() const
{
    const QTextCursor cursor = textCursor();

    const unsigned cursorPosition = cursor.position();
    const SemanticInfo semanticInfo = m_qmlJsEditorDocument->semanticInfo();
    if (!semanticInfo.isValid())
        return;

    const CppComponentValue *cppValue = findCppComponentToInspect(semanticInfo, cursorPosition);
    if (!cppValue) {
        QString title = tr("Code Model Not Available");
        const QString documentId = Constants::QML_JS_EDITOR_PLUGIN + QStringLiteral(".NothingToShow");
        EditorManager::openEditorWithContents(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
                                              tr("Code model not available.").toUtf8(), documentId,
                                              EditorManager::IgnoreNavigationHistory);
        return;
    }

    QString title = tr("Code Model of %1").arg(cppValue->metaObject()->className());
    const QString documentId = Constants::QML_JS_EDITOR_PLUGIN + QStringLiteral(".Class.")
            + cppValue->metaObject()->className();
    IEditor *outputEditor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, QByteArray(),
                documentId, EditorManager::IgnoreNavigationHistory);

    if (!outputEditor)
        return;

    auto widget = qobject_cast<TextEditor::TextEditorWidget *>(outputEditor->widget());
    if (!widget)
        return;

    widget->setReadOnly(true);
    widget->textDocument()->setTemporary(true);
    widget->textDocument()->setSyntaxHighlighter(new QmlJSHighlighter(widget->document()));

    const QString buf = inspectCppComponent(cppValue);
    widget->textDocument()->setPlainText(buf);
}

TextEditorWidget::Link QmlJSEditorWidget::findLinkAt(const QTextCursor &cursor,
                                                             bool /*resolveTarget*/,
                                                             bool /*inNextSplit*/)
{
    const SemanticInfo semanticInfo = m_qmlJsEditorDocument->semanticInfo();
    if (! semanticInfo.isValid())
        return Link();

    const unsigned cursorPosition = cursor.position();

    AST::Node *node = semanticInfo.astNodeAt(cursorPosition);
    QTC_ASSERT(node, return Link());

    if (AST::UiImport *importAst = cast<AST::UiImport *>(node)) {
        // if it's a file import, link to the file
        foreach (const ImportInfo &import, semanticInfo.document->bind()->imports()) {
            if (import.ast() == importAst && import.type() == ImportType::File) {
                TextEditorWidget::Link link(import.path());
                link.linkTextStart = importAst->firstSourceLocation().begin();
                link.linkTextEnd = importAst->lastSourceLocation().end();
                return link;
            }
        }
        return Link();
    }

    // string literals that could refer to a file link to them
    if (StringLiteral *literal = cast<StringLiteral *>(node)) {
        const QString &text = literal->value.toString();
        TextEditorWidget::Link link;
        link.linkTextStart = literal->literalToken.begin();
        link.linkTextEnd = literal->literalToken.end();
        if (semanticInfo.snapshot.document(text)) {
            link.targetFileName = text;
            return link;
        }
        const QString relative = QString::fromLatin1("%1/%2").arg(
                    semanticInfo.document->path(),
                    text);
        if (semanticInfo.snapshot.document(relative)) {
            link.targetFileName = relative;
            return link;
        }
    }

    const ScopeChain scopeChain = semanticInfo.scopeChain(semanticInfo.rangePath(cursorPosition));
    Evaluate evaluator(&scopeChain);
    const Value *value = evaluator.reference(node);

    QString fileName;
    int line = 0, column = 0;

    if (! (value && value->getSourceLocation(&fileName, &line, &column)))
        return Link();

    TextEditorWidget::Link link;
    link.targetFileName = fileName;
    link.targetLine = line;
    link.targetColumn = column - 1; // adjust the column

    if (AST::UiQualifiedId *q = AST::cast<AST::UiQualifiedId *>(node)) {
        for (AST::UiQualifiedId *tail = q; tail; tail = tail->next) {
            if (! tail->next && cursorPosition <= tail->identifierToken.end()) {
                link.linkTextStart = tail->identifierToken.begin();
                link.linkTextEnd = tail->identifierToken.end();
                return link;
            }
        }

    } else if (AST::IdentifierExpression *id = AST::cast<AST::IdentifierExpression *>(node)) {
        link.linkTextStart = id->firstSourceLocation().begin();
        link.linkTextEnd = id->lastSourceLocation().end();
        return link;

    } else if (AST::FieldMemberExpression *mem = AST::cast<AST::FieldMemberExpression *>(node)) {
        link.linkTextStart = mem->lastSourceLocation().begin();
        link.linkTextEnd = mem->lastSourceLocation().end();
        return link;
    }

    return Link();
}

void QmlJSEditorWidget::findUsages()
{
    m_findReferences->findUsages(textDocument()->filePath().toString(), textCursor().position());
}

void QmlJSEditorWidget::renameUsages()
{
    m_findReferences->renameUsages(textDocument()->filePath().toString(), textCursor().position());
}

void QmlJSEditorWidget::showContextPane()
{
    const SemanticInfo info = m_qmlJsEditorDocument->semanticInfo();
    if (m_contextPane && info.isValid()) {
        Node *newNode = info.declaringMemberNoProperties(position());
        ScopeChain scopeChain = info.scopeChain(info.rangePath(position()));
        m_contextPane->apply(this, info.document,
                             &scopeChain,
                             newNode, false, true);
        m_oldCursorPosition = position();
        setRefactorMarkers(removeMarkersOfType<QtQuickToolbarMarker>(refactorMarkers()));
    }
}

void QmlJSEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    m_blockOutLineCursorChanges = true;
    QPointer<QMenu> menu(new QMenu(this));

    QMenu *refactoringMenu = new QMenu(tr("Refactoring"), menu);

    if (!m_qmlJsEditorDocument->isSemanticInfoOutdated()) {
        AssistInterface *interface = createAssistInterface(QuickFix, ExplicitlyInvoked);
        if (interface) {
            QScopedPointer<IAssistProcessor> processor(
                        QmlJSEditorPlugin::instance()->quickFixAssistProvider()->createProcessor());
            QScopedPointer<IAssistProposal> proposal(processor->perform(interface));
            if (!proposal.isNull()) {
                GenericProposalModel *model = static_cast<GenericProposalModel *>(proposal->model());
                for (int index = 0; index < model->size(); ++index) {
                    AssistProposalItem *item = static_cast<AssistProposalItem *>(model->proposalItem(index));
                    QuickFixOperation::Ptr op = item->data().value<QuickFixOperation::Ptr>();
                    QAction *action = refactoringMenu->addAction(op->description());
                    connect(action, &QAction::triggered, this, [op]() { op->perform(); });
                }
                delete model;
            }
        }
    }

    refactoringMenu->setEnabled(!refactoringMenu->isEmpty());

    if (ActionContainer *mcontext = ActionManager::actionContainer(Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions()) {
            menu->addAction(action);
            if (action->objectName() == QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT))
                menu->addMenu(refactoringMenu);
            if (action->objectName() == QLatin1String(Constants::SHOW_QT_QUICK_HELPER)) {
                bool enabled = m_contextPane->isAvailable(
                            this, m_qmlJsEditorDocument->semanticInfo().document,
                            m_qmlJsEditorDocument->semanticInfo().declaringMemberNoProperties(position()));
                action->setEnabled(enabled);
            }
        }
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    delete menu;
    m_blockOutLineCursorChanges = false;
}

bool QmlJSEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_contextPane) {
            if (hideContextPane()) {
                e->accept();
                return true;
            }
        }
        break;
    default:
        break;
    }

    return TextEditorWidget::event(e);
}


void QmlJSEditorWidget::wheelEvent(QWheelEvent *event)
{
    bool visible = false;
    if (m_contextPane && m_contextPane->widget()->isVisible())
        visible = true;

    TextEditorWidget::wheelEvent(event);

    if (visible)
        m_contextPane->apply(this, m_qmlJsEditorDocument->semanticInfo().document, 0,
                             m_qmlJsEditorDocument->semanticInfo().declaringMemberNoProperties(m_oldCursorPosition),
                             false, true);
}

void QmlJSEditorWidget::resizeEvent(QResizeEvent *event)
{
    TextEditorWidget::resizeEvent(event);
    hideContextPane();
}

 void QmlJSEditorWidget::scrollContentsBy(int dx, int dy)
 {
     TextEditorWidget::scrollContentsBy(dx, dy);
     hideContextPane();
 }

QmlJSEditorDocument *QmlJSEditorWidget::qmlJsEditorDocument() const
{
    return m_qmlJsEditorDocument;
}

void QmlJSEditorWidget::semanticInfoUpdated(const SemanticInfo &semanticInfo)
{
    if (isVisible()) {
         // trigger semantic highlighting and model update if necessary
        textDocument()->triggerPendingUpdates();
    }

    if (m_contextPane) {
        Node *newNode = semanticInfo.declaringMemberNoProperties(position());
        if (newNode) {
            m_contextPane->apply(this, semanticInfo.document, 0, newNode, true);
            m_contextPaneTimer.start(); //update text marker
        }
    }

    updateUses();
}

void QmlJSEditorWidget::onRefactorMarkerClicked(const RefactorMarker &marker)
{
    if (marker.data.canConvert<QtQuickToolbarMarker>())
        showContextPane();
}

QModelIndex QmlJSEditorWidget::indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;

    QmlOutlineModel *model = m_qmlJsEditorDocument->outlineModel();
    const int rowCount = model->rowCount(rootIndex);
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex childIndex = model->index(i, 0, rootIndex);
        AST::SourceLocation location = model->sourceLocation(childIndex);

        if ((cursorPosition >= location.offset)
              && (cursorPosition <= location.offset + location.length)) {
            lastIndex = childIndex;
            break;
        }
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(cursorPosition, lastIndex);
    }
    return lastIndex;
}

bool QmlJSEditorWidget::hideContextPane()
{
    bool b = (m_contextPane) && m_contextPane->widget()->isVisible();
    if (b)
        m_contextPane->apply(this, m_qmlJsEditorDocument->semanticInfo().document, 0, 0, false);
    return b;
}

AssistInterface *QmlJSEditorWidget::createAssistInterface(
    AssistKind assistKind,
    AssistReason reason) const
{
    if (assistKind == Completion) {
        return new QmlJSCompletionAssistInterface(document(),
                                                  position(),
                                                  textDocument()->filePath().toString(),
                                                  reason,
                                                  m_qmlJsEditorDocument->semanticInfo());
    } else if (assistKind == QuickFix) {
        return new QmlJSQuickFixAssistInterface(const_cast<QmlJSEditorWidget *>(this), reason);
    }
    return 0;
}

QString QmlJSEditorWidget::foldReplacementText(const QTextBlock &block) const
{
    const int curlyIndex = block.text().indexOf(QLatin1Char('{'));

    if (curlyIndex != -1 && m_qmlJsEditorDocument->semanticInfo().isValid()) {
        const int pos = block.position() + curlyIndex;
        Node *node = m_qmlJsEditorDocument->semanticInfo().rangeAt(pos);

        const QString objectId = idOfObject(node);
        if (!objectId.isEmpty())
            return QLatin1String("id: ") + objectId + QLatin1String("...");
    }

    return TextEditorWidget::foldReplacementText(block);
}


//
// QmlJSEditor
//

QmlJSEditor::QmlJSEditor()
{
    addContext(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID);
}

bool QmlJSEditor::isDesignModePreferred() const
{

    bool alwaysPreferDesignMode = false;
    // always prefer design mode for .ui.qml files
    if (textDocument() && textDocument()->mimeType() == QLatin1String(QmlJSTools::Constants::QMLUI_MIMETYPE))
        alwaysPreferDesignMode = true;

    // stay in design mode if we are there
    Id mode = ModeManager::currentMode();
    return alwaysPreferDesignMode || mode == Core::Constants::MODE_DESIGN;
}


//
// QmlJSEditorFactory
//

QmlJSEditorFactory::QmlJSEditorFactory()
{
    setId(Constants::C_QMLJSEDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::C_QMLJSEDITOR_DISPLAY_NAME));

    addMimeType(QmlJSTools::Constants::QML_MIMETYPE);
    addMimeType(QmlJSTools::Constants::QMLPROJECT_MIMETYPE);
    addMimeType(QmlJSTools::Constants::QBS_MIMETYPE);
    addMimeType(QmlJSTools::Constants::QMLTYPES_MIMETYPE);
    addMimeType(QmlJSTools::Constants::JS_MIMETYPE);
    addMimeType(QmlJSTools::Constants::JSON_MIMETYPE);

    setDocumentCreator([]() { return new QmlJSEditorDocument; });
    setEditorWidgetCreator([]() { return new QmlJSEditorWidget; });
    setEditorCreator([]() { return new QmlJSEditor; });
    setAutoCompleterCreator([]() { return new AutoCompleter; });
    setCommentStyle(Utils::CommentDefinition::CppStyle);
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);

    addHoverHandler(new QmlJSHoverHandler);
    setCompletionAssistProvider(new QmlJSCompletionAssistProvider);

    setEditorActionHandlers(TextEditorActionHandler::Format
        | TextEditorActionHandler::UnCommentSelection
        | TextEditorActionHandler::UnCollapseAll
        | TextEditorActionHandler::FollowSymbolUnderCursor);
}

} // namespace Internal
} // namespace QmlJSEditor
