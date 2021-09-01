/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppeditorwidget.h"

#include "cppautocompleter.h"
#include "cppcanonicalsymbol.h"
#include "cppchecksymbols.h"
#include "cppcodeformatter.h"
#include "cppcodemodelsettings.h"
#include "cppcompletionassistprovider.h"
#include "doxygengenerator.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditoroutline.h"
#include "cppeditorplugin.h"
#include "cppfunctiondecldeflink.h"
#include "cpphighlighter.h"
#include "cpplocalrenaming.h"
#include "cppminimizableinfobars.h"
#include "cppmodelmanager.h"
#include "cpppreprocessordialog.h"
#include "cppsemanticinfo.h"
#include "cppselectionchanger.h"
#include "cppqtstyleindenter.h"
#include "cppquickfixassistant.h"
#include "cpptoolsreuse.h"
#include "cpptoolssettings.h"
#include "cppuseselectionsupdater.h"
#include "cppworkingcopy.h"
#include "followsymbolinterface.h"
#include "refactoringengineinterface.h"
#include "symbolfinder.h"

#include <clangsupport/sourcelocationscontainer.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <texteditor/basefilefind.h>
#include <texteditor/behaviorsettings.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/commentssettings.h>
#include <texteditor/completionsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/refactoroverlay.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <projectexplorer/projecttree.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/FastPreprocessor.h>
#include <cplusplus/MatchingText.h>
#include <utils/infobar.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QMenu>
#include <QPointer>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

enum { UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL = 200 };

using namespace Core;
using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

namespace {

bool isStartOfDoxygenComment(const QTextCursor &cursor)
{
    const int pos = cursor.position();

    QTextDocument *document = cursor.document();
    QString comment = QString(document->characterAt(pos - 3))
            + document->characterAt(pos - 2)
            + document->characterAt(pos - 1);

    return comment == QLatin1String("/**")
           || comment == QLatin1String("/*!")
           || comment == QLatin1String("///")
           || comment == QLatin1String("//!");
}

DoxygenGenerator::DocumentationStyle doxygenStyle(const QTextCursor &cursor,
                                                  const QTextDocument *doc)
{
    const int pos = cursor.position();

    QString comment = QString(doc->characterAt(pos - 3))
            + doc->characterAt(pos - 2)
            + doc->characterAt(pos - 1);

    if (comment == QLatin1String("/**"))
        return DoxygenGenerator::JavaStyle;
    else if (comment == QLatin1String("/*!"))
        return DoxygenGenerator::QtStyle;
    else if (comment == QLatin1String("///"))
        return DoxygenGenerator::CppStyleA;
    else
        return DoxygenGenerator::CppStyleB;
}

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
    return text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!"));
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
    return text.startsWith(QLatin1String("///")) || text.startsWith(QLatin1String("//!"));
}

bool isCppStyleContinuation(const QTextCursor& cursor)
{
    return isPreviousLineCppStyleComment(cursor) || isNextLineCppStyleComment(cursor);
}

bool lineStartsWithCppDoxygenCommentAndCursorIsAfter(const QTextCursor &cursor,
                                                     const QTextDocument *doc)
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

bool isCursorAfterNonNestedCppStyleComment(const QTextCursor &cursor,
                                           TextEditor::TextEditorWidget *editorWidget)
{
    QTextDocument *document = editorWidget->document();
    QTextCursor cursorBeforeCppComment(cursor);
    while (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/')
           && cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter)) {
    }

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    if (document->characterAt(cursorBeforeCppComment.position()) != QLatin1Char('/'))
        return false;

    if (!cursorBeforeCppComment.movePosition(QTextCursor::PreviousCharacter))
        return false;

    return !CPlusPlus::MatchingText::isInCommentHelper(cursorBeforeCppComment);
}

bool handleDoxygenCppStyleContinuation(QTextCursor &cursor)
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
    if (offset + 3 > text.size())
        return false;
    const QStringView commentMarker = QStringView(text).mid(offset, 3);
    if (commentMarker != QLatin1String("///") && commentMarker != QLatin1String("//!"))
        return false;

    QString newLine(QLatin1Char('\n'));
    newLine.append(text.left(offset)); // indent correctly
    newLine.append(commentMarker.toString());
    newLine.append(QLatin1Char(' '));

    cursor.insertText(newLine);
    return true;
}

bool handleDoxygenContinuation(QTextCursor &cursor,
                               TextEditor::TextEditorWidget *editorWidget,
                               const bool enableDoxygen,
                               const bool leadingAsterisks)
{
    const QTextDocument *doc = editorWidget->document();

    // It might be a continuation if:
    // a) current line starts with /// or //! and cursor is positioned after the comment
    // b) current line is in the middle of a multi-line Qt or Java style comment

    if (!cursor.atEnd()) {
        if (enableDoxygen && lineStartsWithCppDoxygenCommentAndCursorIsAfter(cursor, doc))
            return handleDoxygenCppStyleContinuation(cursor);

        if (isCursorAfterNonNestedCppStyleComment(cursor, editorWidget))
            return false;
    }

    // We continue the comment if the cursor is after a comment's line asterisk and if
    // there's no asterisk immediately after the cursor (that would already be considered
    // a leading asterisk).
    int offset = 0;
    const int blockPos = cursor.positionInBlock();
    const QString &currentLine = cursor.block().text();
    for (; offset < blockPos; ++offset) {
        if (!currentLine.at(offset).isSpace())
            break;
    }

    // In case we don't need to insert leading asteriskses, this code will be run once (right after
    // hitting enter on the line containing '/*'). It will insert a continuation without an
    // asterisk, but with an extra space. After that, the normal indenting will take over and do the
    // Right Thing <TM>.
    if (offset < blockPos
            && (currentLine.at(offset) == QLatin1Char('*')
                || (offset < blockPos - 1
                    && currentLine.at(offset) == QLatin1Char('/')
                    && currentLine.at(offset + 1) == QLatin1Char('*')))) {
        // Ok, so the line started with an '*' or '/*'
        int followinPos = blockPos;
        // Now search for the first non-whitespace character to align to:
        for (; followinPos < currentLine.length(); ++followinPos) {
            if (!currentLine.at(followinPos).isSpace())
                break;
        }
        if (followinPos == currentLine.length() // a)
                || currentLine.at(followinPos) != QLatin1Char('*')) { // b)
            // So either a) the line ended after a '*' and we need to insert a continuation, or
            // b) we found the start of some text and we want to align the continuation to that.
            QString newLine(QLatin1Char('\n'));
            QTextCursor c(cursor);
            c.movePosition(QTextCursor::StartOfBlock);
            c.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, offset);
            newLine.append(c.selectedText());
            if (currentLine.at(offset) == QLatin1Char('/')) {
                if (leadingAsterisks)
                    newLine.append(QLatin1String(" * "));
                else
                    newLine.append(QLatin1String("   "));
                offset += 3;
            } else {
                // If '*' is not within a comment, skip.
                QTextCursor cursorOnFirstNonWhiteSpace(cursor);
                const int positionOnFirstNonWhiteSpace = cursor.position() - blockPos + offset;
                cursorOnFirstNonWhiteSpace.setPosition(positionOnFirstNonWhiteSpace);
                if (!CPlusPlus::MatchingText::isInCommentHelper(cursorOnFirstNonWhiteSpace))
                    return false;

                // ...otherwise do the continuation
                int start = offset;
                while (offset < blockPos && currentLine.at(offset) == QLatin1Char('*'))
                    ++offset;
                const QChar ch = leadingAsterisks ? QLatin1Char('*') : QLatin1Char(' ');
                newLine.append(QString(offset - start, ch));
            }
            for (; offset < blockPos && currentLine.at(offset) == ' '; ++offset)
                newLine.append(QLatin1Char(' '));
            cursor.insertText(newLine);
            return true;
        }
    }

    return false;
}

static bool trySplitComment(TextEditor::TextEditorWidget *editorWidget,
                     const CPlusPlus::Snapshot &snapshot)
{
    const TextEditor::CommentsSettings &settings = CppToolsSettings::instance()->commentsSettings();
    if (!settings.m_enableDoxygen && !settings.m_leadingAsterisks)
        return false;

    QTextCursor cursor = editorWidget->textCursor();
    if (!CPlusPlus::MatchingText::isInCommentHelper(cursor))
        return false;

    // We are interested on two particular cases:
    //   1) The cursor is right after a /**, /*!, /// or ///! and the user pressed enter.
    //      If Doxygen is enabled we need to generate an entire comment block.
    //   2) The cursor is already in the middle of a multi-line comment and the user pressed
    //      enter. If leading asterisk(s) is set we need to write a comment continuation
    //      with those.

    if (settings.m_enableDoxygen && cursor.positionInBlock() >= 3) {
        const int pos = cursor.position();
        if (isStartOfDoxygenComment(cursor)) {
            QTextDocument *textDocument = editorWidget->document();
            DoxygenGenerator::DocumentationStyle style = doxygenStyle(cursor, textDocument);

            // Check if we're already in a CppStyle Doxygen comment => continuation
            // Needs special handling since CppStyle does not have start and end markers
            if ((style == DoxygenGenerator::CppStyleA || style == DoxygenGenerator::CppStyleB)
                    && isCppStyleContinuation(cursor)) {
                return handleDoxygenCppStyleContinuation(cursor);
            }

            DoxygenGenerator doxygen;
            doxygen.setStyle(style);
            doxygen.setAddLeadingAsterisks(settings.m_leadingAsterisks);
            doxygen.setGenerateBrief(settings.m_generateBrief);
            doxygen.setStartComment(false);

            // Move until we reach any possibly meaningful content.
            while (textDocument->characterAt(cursor.position()).isSpace()
                   && cursor.movePosition(QTextCursor::NextCharacter)) {
            }

            if (!cursor.atEnd()) {
                const QString &comment = doxygen.generate(cursor,
                                                          snapshot,
                                                          editorWidget->textDocument()->filePath());
                if (!comment.isEmpty()) {
                    cursor.beginEditBlock();
                    cursor.setPosition(pos);
                    cursor.insertText(comment);
                    cursor.setPosition(pos - 3, QTextCursor::KeepAnchor);
                    editorWidget->textDocument()->autoIndent(cursor);
                    cursor.endEditBlock();
                    return true;
                }
                cursor.setPosition(pos);
            }
        }
    } // right after first doxygen comment

    return handleDoxygenContinuation(cursor,
                                     editorWidget,
                                     settings.m_enableDoxygen,
                                     settings.m_leadingAsterisks);
}

} // anonymous namespace

class CppEditorWidgetPrivate
{
public:
    CppEditorWidgetPrivate(CppEditorWidget *q);

    bool shouldOfferOutline() const { return CppModelManager::supportsOutline(m_cppEditorDocument); }

public:
    QPointer<CppModelManager> m_modelManager;

    CppEditorDocument *m_cppEditorDocument;
    CppEditorOutline *m_cppEditorOutline = nullptr;
    QAction *m_outlineAction = nullptr;
    QTimer m_outlineTimer;

    QTimer m_updateFunctionDeclDefLinkTimer;
    SemanticInfo m_lastSemanticInfo;

    FunctionDeclDefLinkFinder *m_declDefLinkFinder;
    QSharedPointer<FunctionDeclDefLink> m_declDefLink;

    QAction *m_parseContextAction = nullptr;
    ParseContextWidget *m_parseContextWidget = nullptr;
    QToolButton *m_preprocessorButton = nullptr;
    MinimizableInfoBars::Actions m_showInfoBarActions;

    CppLocalRenaming m_localRenaming;
    CppUseSelectionsUpdater m_useSelectionsUpdater;
    CppSelectionChanger m_cppSelectionChanger;
    bool inTestMode = false;
};

CppEditorWidgetPrivate::CppEditorWidgetPrivate(CppEditorWidget *q)
    : m_modelManager(CppModelManager::instance())
    , m_cppEditorDocument(qobject_cast<CppEditorDocument *>(q->textDocument()))
    , m_declDefLinkFinder(new FunctionDeclDefLinkFinder(q))
    , m_localRenaming(q)
    , m_useSelectionsUpdater(q)
    , m_cppSelectionChanger()
{}
} // namespace Internal

using namespace Internal;

CppEditorWidget::CppEditorWidget()
    : d(new CppEditorWidgetPrivate(this))
{
    qRegisterMetaType<SemanticInfo>("SemanticInfo");
}

void CppEditorWidget::finalizeInitialization()
{
    d->m_cppEditorDocument = qobject_cast<CppEditorDocument *>(textDocument());

    setLanguageSettingsId(Constants::CPP_SETTINGS_ID);

    // clang-format off
    // function combo box sorting
    d->m_cppEditorOutline = new CppEditorOutline(this);

    // TODO: Nobody emits this signal... Remove?
    connect(CppEditorPlugin::instance(), &CppEditorPlugin::outlineSortingChanged,
            outline(), &CppEditorOutline::setSorted);

    connect(d->m_cppEditorDocument, &CppEditorDocument::codeWarningsUpdated,
            this, &CppEditorWidget::onCodeWarningsUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::ifdefedOutBlocksUpdated,
            this, &CppEditorWidget::onIfdefedOutBlocksUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::cppDocumentUpdated,
            this, &CppEditorWidget::onCppDocumentUpdated);
    connect(d->m_cppEditorDocument, &CppEditorDocument::semanticInfoUpdated,
            this, [this](const SemanticInfo &info) { updateSemanticInfo(info); });

    connect(d->m_declDefLinkFinder, &FunctionDeclDefLinkFinder::foundLink,
            this, &CppEditorWidget::onFunctionDeclDefLinkFound);

    connect(&d->m_useSelectionsUpdater,
            &CppUseSelectionsUpdater::selectionsForVariableUnderCursorUpdated,
            &d->m_localRenaming,
            &CppLocalRenaming::updateSelectionsForVariableUnderCursor);

    connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished, this,
            [this] (SemanticInfo::LocalUseMap localUses, bool success) {
                if (success) {
                    d->m_lastSemanticInfo.localUsesUpdated = true;
                    d->m_lastSemanticInfo.localUses = localUses;
                }
    });

    connect(document(), &QTextDocument::contentsChange,
            &d->m_localRenaming, &CppLocalRenaming::onContentsChangeOfEditorWidgetDocument);
    connect(&d->m_localRenaming, &CppLocalRenaming::finished, [this] {
        cppEditorDocument()->recalculateSemanticInfoDetached();
    });
    connect(&d->m_localRenaming, &CppLocalRenaming::processKeyPressNormally,
            this, &CppEditorWidget::processKeyNormally);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        if (d->shouldOfferOutline())
            d->m_cppEditorOutline->updateIndex();
    });

    connect(cppEditorDocument(), &CppEditorDocument::preprocessorSettingsChanged, this,
            [this](bool customSettings) {
        updateWidgetHighlighting(d->m_preprocessorButton, customSettings);
    });

    // set up function declaration - definition link
    d->m_updateFunctionDeclDefLinkTimer.setSingleShot(true);
    d->m_updateFunctionDeclDefLinkTimer.setInterval(UPDATE_FUNCTION_DECL_DEF_LINK_INTERVAL);
    connect(&d->m_updateFunctionDeclDefLinkTimer, &QTimer::timeout,
            this, &CppEditorWidget::updateFunctionDeclDefLinkNow);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);
    connect(this, &QPlainTextEdit::textChanged, this, &CppEditorWidget::updateFunctionDeclDefLink);

    // set up the use highlighitng
    connect(this, &CppEditorWidget::cursorPositionChanged, this, [this]() {
        if (!d->m_localRenaming.isActive())
            d->m_useSelectionsUpdater.scheduleUpdate();

        // Notify selection expander about the changed cursor.
        d->m_cppSelectionChanger.onCursorPositionChanged(textCursor());
    });

    // Toolbar: Parse context
    ParseContextModel &parseContextModel = cppEditorDocument()->parseContextModel();
    d->m_parseContextWidget = new ParseContextWidget(parseContextModel, this);
    d->m_parseContextAction = insertExtraToolBarWidget(TextEditorWidget::Left,
                                                       d->m_parseContextWidget);
    d->m_parseContextAction->setVisible(false);
    connect(&parseContextModel, &ParseContextModel::updated,
            this, [this](bool areMultipleAvailable) {
        d->m_parseContextAction->setVisible(areMultipleAvailable);
    });

    // Toolbar: Outline/Overview combo box
    d->m_outlineAction = insertExtraToolBarWidget(TextEditorWidget::Left,
                                                  d->m_cppEditorOutline->widget());

    // clang-format on
    // Toolbar: '#' Button
    // TODO: Make "Additional Preprocessor Directives" also useful with Clang Code Model.
    if (!d->m_modelManager->isClangCodeModelActive()) {
        d->m_preprocessorButton = new QToolButton(this);
        d->m_preprocessorButton->setText(QLatin1String("#"));
        Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
        connect(cmd, &Command::keySequenceChanged,
                this, &CppEditorWidget::updatePreprocessorButtonTooltip);
        updatePreprocessorButtonTooltip();
        connect(d->m_preprocessorButton, &QAbstractButton::clicked,
                this, &CppEditorWidget::showPreProcessorWidget);

        insertExtraToolBarWidget(TextEditorWidget::Left, d->m_preprocessorButton);
    }

    // Toolbar: Actions to show minimized info bars
    d->m_showInfoBarActions = MinimizableInfoBars::createShowInfoBarActions([this](QWidget *w) {
        return this->insertExtraToolBarWidget(TextEditorWidget::Left, w);
    });
    connect(&cppEditorDocument()->minimizableInfoBars(), &MinimizableInfoBars::showAction,
            this, &CppEditorWidget::onShowInfoBarAction);

    d->m_outlineTimer.setInterval(5000);
    d->m_outlineTimer.setSingleShot(true);
    connect(&d->m_outlineTimer, &QTimer::timeout, this, [this] {
        d->m_outlineAction->setVisible(d->shouldOfferOutline());
        if (d->m_outlineAction->isVisible()) {
            d->m_cppEditorOutline->update();
            d->m_cppEditorOutline->updateIndex();
        }
    });
    connect(&ClangdSettings::instance(), &ClangdSettings::changed,
            &d->m_outlineTimer, qOverload<>(&QTimer::start));
    connect(d->m_cppEditorDocument, &CppEditorDocument::changed,
            &d->m_outlineTimer, qOverload<>(&QTimer::start));
}

void CppEditorWidget::finalizeInitializationAfterDuplication(TextEditorWidget *other)
{
    QTC_ASSERT(other, return);
    auto cppEditorWidget = qobject_cast<CppEditorWidget *>(other);
    QTC_ASSERT(cppEditorWidget, return);

    if (cppEditorWidget->isSemanticInfoValidExceptLocalUses())
        updateSemanticInfo(cppEditorWidget->semanticInfo());
    if (d->shouldOfferOutline())
        d->m_cppEditorOutline->update();
    const Id selectionKind = CodeWarningsSelection;
    setExtraSelections(selectionKind, cppEditorWidget->extraSelections(selectionKind));

    if (isWidgetHighlighted(cppEditorWidget->d->m_preprocessorButton))
        updateWidgetHighlighting(d->m_preprocessorButton, true);

    d->m_parseContextWidget->syncToModel();
    d->m_parseContextAction->setVisible(
                d->m_cppEditorDocument->parseContextModel().areMultipleAvailable());
}

void CppEditorWidget::setProposals(const TextEditor::IAssistProposal *immediateProposal,
                                   const TextEditor::IAssistProposal *finalProposal)
{
    QTC_ASSERT(isInTestMode(), return);
#ifdef WITH_TESTS
    emit proposalsReady(immediateProposal, finalProposal);
#endif
}

CppEditorWidget::~CppEditorWidget() = default;

CppEditorDocument *CppEditorWidget::cppEditorDocument() const
{
    return d->m_cppEditorDocument;
}

CppEditorOutline *CppEditorWidget::outline() const
{
    return d->m_cppEditorOutline;
}

void CppEditorWidget::paste()
{
    if (d->m_localRenaming.handlePaste())
        return;

    TextEditorWidget::paste();
}

void CppEditorWidget::cut()
{
    if (d->m_localRenaming.handleCut())
        return;

    TextEditorWidget::cut();
}

void CppEditorWidget::selectAll()
{
    if (d->m_localRenaming.handleSelectAll())
        return;

    TextEditorWidget::selectAll();
}

void CppEditorWidget::onCppDocumentUpdated()
{
    if (d->shouldOfferOutline())
        d->m_cppEditorOutline->update();
}

void CppEditorWidget::onCodeWarningsUpdated(unsigned revision,
                                            const QList<QTextEdit::ExtraSelection> selections,
                                            const RefactorMarkers &refactorMarkers)
{
    if (revision != documentRevision())
        return;

    setExtraSelections(TextEditorWidget::CodeWarningsSelection,
                       unselectLeadingWhitespace(selections));
    setRefactorMarkers(refactorMarkers + RefactorMarker::filterOutType(
            this->refactorMarkers(), Constants::CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID));
}

void CppEditorWidget::onIfdefedOutBlocksUpdated(unsigned revision,
                                                const QList<BlockRange> ifdefedOutBlocks)
{
    if (revision != documentRevision())
        return;
    setIfdefedOutBlocks(ifdefedOutBlocks);
}

void CppEditorWidget::onShowInfoBarAction(const Id &id, bool show)
{
    QAction *action = d->m_showInfoBarActions.value(id);
    QTC_ASSERT(action, return);
    action->setVisible(show);
}

static QString getDocumentLine(QTextDocument *document, int line)
{
    if (document)
        return document->findBlockByNumber(line - 1).text();

    return {};
}

static std::unique_ptr<QTextDocument> getCurrentDocument(const QString &path)
{
    const QTextCodec *defaultCodec = Core::EditorManager::defaultTextCodec();
    QString contents;
    Utils::TextFileFormat format;
    QString error;
    if (Utils::TextFileFormat::readFile(Utils::FilePath::fromString(path),
                                        defaultCodec,
                                        &contents,
                                        &format,
                                        &error)
        != Utils::TextFileFormat::ReadSuccess) {
        qWarning() << "Error reading file " << path << " : " << error;
        return {};
    }

    return std::make_unique<QTextDocument>(contents);
}

static void onReplaceUsagesClicked(const QString &text,
                                   const QList<SearchResultItem> &items,
                                   bool preserveCase)
{
    CppModelManager *modelManager = CppModelManager::instance();
    if (!modelManager)
        return;

    const FilePaths filePaths = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);
    if (!filePaths.isEmpty()) {
        modelManager->updateSourceFiles(Utils::transform<QSet>(filePaths, &FilePath::toString));
        SearchResultWindow::instance()->hide();
    }
}

static QTextDocument *getOpenDocument(const QString &path)
{
    const IDocument *document = DocumentModel::documentForFilePath(FilePath::fromString(path));
    if (document)
        return qobject_cast<const TextDocument *>(document)->document();

    return {};
}

static void addSearchResults(Usages usages, SearchResult &search, const QString &text)
{
    std::sort(usages.begin(), usages.end());

    std::unique_ptr<QTextDocument> currentDocument;
    QString lastPath;

    for (const Usage &usage : usages) {
        QTextDocument *document = getOpenDocument(usage.path);

        if (!document) {
            if (usage.path != lastPath) {
                currentDocument = getCurrentDocument(usage.path);
                lastPath = usage.path;
            }
            document = currentDocument.get();
        }

        const QString lineContent = getDocumentLine(document, usage.line);

        if (!lineContent.isEmpty()) {
            Search::TextRange range{Search::TextPosition(usage.line, usage.column - 1),
                                    Search::TextPosition(usage.line, usage.column + text.length() - 1)};
            SearchResultItem item;
            item.setFilePath(FilePath::fromString(usage.path));
            item.setLineText(lineContent);
            item.setMainRange(range);
            item.setUseTextEditorFont(true);
            search.addResult(item);
        }
    }
}

static void findRenameCallback(CppEditorWidget *widget,
                               const QTextCursor &baseCursor,
                               const Usages &usages,
                               bool rename = false,
                               const QString &replacement = QString())
{
    QTextCursor cursor = Utils::Text::wordStartCursor(baseCursor);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString text = cursor.selectedText();
    SearchResultWindow::SearchMode mode = SearchResultWindow::SearchOnly;
    if (rename)
        mode = SearchResultWindow::SearchAndReplace;
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                QObject::tr("C++ Usages:"),
                QString(),
                text,
                mode,
                SearchResultWindow::PreserveCaseDisabled,
                QLatin1String("CppEditor"));
    search->setTextToReplace(replacement);
    search->setSearchAgainSupported(true);
    QObject::connect(search, &SearchResult::replaceButtonClicked, &onReplaceUsagesClicked);
    QObject::connect(search, &SearchResult::searchAgainRequested,
                     [widget, rename, replacement, baseCursor]() {
        rename ? widget->renameUsages(replacement, baseCursor) : widget->findUsages(baseCursor);
    });

    addSearchResults(usages, *search, text);

    search->finishSearch(false);
    QObject::connect(search, &SearchResult::activated,
                     [](const Core::SearchResultItem& item) {
                         Core::EditorManager::openEditorAtSearchResult(item);
                     });
    search->popup();
}

void CppEditorWidget::findUsages()
{
    findUsages(textCursor());
}

void CppEditorWidget::findUsages(QTextCursor cursor)
{
    // 'this' in cursorInEditor is never used (and must never be used) asynchronously.
    const CursorInEditor cursorInEditor{cursor, textDocument()->filePath(), this,
                textDocument()};
    QPointer<CppEditorWidget> cppEditorWidget = this;
    d->m_modelManager->findUsages(cursorInEditor,
                                  [=](const Usages &usages) {
                                      if (!cppEditorWidget)
                                          return;
                                      findRenameCallback(cppEditorWidget.data(), cursor, usages);
                                  });
}

void CppEditorWidget::renameUsages(const QString &replacement, QTextCursor cursor)
{
    if (cursor.isNull())
        cursor = textCursor();
    CursorInEditor cursorInEditor{cursor, textDocument()->filePath(), this,
                textDocument()};
    QPointer<CppEditorWidget> cppEditorWidget = this;
    d->m_modelManager->globalRename(cursorInEditor,
                                    [=](const Usages &usages) {
                                        if (!cppEditorWidget)
                                            return;
                                        findRenameCallback(cppEditorWidget.data(), cursor, usages,
                                                           true, replacement);
                                    },
                                    replacement);
}

bool CppEditorWidget::selectBlockUp()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockUp();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ExpandSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

bool CppEditorWidget::selectBlockDown()
{
    if (!behaviorSettings().m_smartSelectionChanging)
        return TextEditorWidget::selectBlockDown();

    QTextCursor cursor = textCursor();
    d->m_cppSelectionChanger.startChangeSelection();
    const bool changed = d->m_cppSelectionChanger
                             .changeSelection(CppSelectionChanger::ShrinkSelection,
                                              cursor,
                                              d->m_lastSemanticInfo.doc);
    if (changed)
        setTextCursor(cursor);
    d->m_cppSelectionChanger.stopChangeSelection();

    return changed;
}

void CppEditorWidget::updateWidgetHighlighting(QWidget *widget, bool highlight)
{
    if (!widget)
        return;

    widget->setProperty("highlightWidget", highlight);
    widget->update();
}

bool CppEditorWidget::isWidgetHighlighted(QWidget *widget)
{
    return widget ? widget->property("highlightWidget").toBool() : false;
}

namespace {

QList<ProjectPart::ConstPtr> fetchProjectParts(CppModelManager *modelManager,
                                               const Utils::FilePath &filePath)
{
    QList<ProjectPart::ConstPtr> projectParts = modelManager->projectPart(filePath);

    if (projectParts.isEmpty())
        projectParts = modelManager->projectPartFromDependencies(filePath);
    if (projectParts.isEmpty())
        projectParts.append(modelManager->fallbackProjectPart());

    return projectParts;
}

const ProjectPart *findProjectPartForCurrentProject(
        const QList<ProjectPart::ConstPtr> &projectParts,
        ProjectExplorer::Project *currentProject)
{
    const auto found = std::find_if(projectParts.cbegin(),
                              projectParts.cend(),
                              [&](const ProjectPart::ConstPtr &projectPart) {
                                  return projectPart->belongsToProject(currentProject);
                              });

    if (found != projectParts.cend())
        return (*found).data();

    return nullptr;
}

} // namespace

const ProjectPart *CppEditorWidget::projectPart() const
{
    if (!d->m_modelManager)
        return nullptr;

    auto projectParts = fetchProjectParts(d->m_modelManager, textDocument()->filePath());

    return findProjectPartForCurrentProject(projectParts,
                                            ProjectExplorer::ProjectTree::currentProject());
}

namespace {

using ClangBackEnd::SourceLocationContainer;
using Utils::Text::selectAt;

QTextCharFormat occurrencesTextCharFormat()
{
    using TextEditor::TextEditorSettings;

    return TextEditorSettings::fontSettings().toTextCharFormat(TextEditor::C_OCCURRENCES);
}

QList<QTextEdit::ExtraSelection> sourceLocationsToExtraSelections(
    const std::vector<SourceLocationContainer> &sourceLocations,
    uint selectionLength,
    CppEditorWidget *cppEditorWidget)
{
    const auto textCharFormat = occurrencesTextCharFormat();

    QList<QTextEdit::ExtraSelection> selections;
    selections.reserve(int(sourceLocations.size()));

    auto sourceLocationToExtraSelection = [&](const SourceLocationContainer &sourceLocation) {
        QTextEdit::ExtraSelection selection;

        selection.cursor = selectAt(cppEditorWidget->textCursor(),
                                    sourceLocation.line,
                                    sourceLocation.column,
                                    selectionLength);
        selection.format = textCharFormat;

        return selection;
    };

    std::transform(sourceLocations.begin(),
                   sourceLocations.end(),
                   std::back_inserter(selections),
                   sourceLocationToExtraSelection);

    return selections;
};

}

void CppEditorWidget::renameSymbolUnderCursor()
{
    using ClangBackEnd::SourceLocationsContainer;

    const ProjectPart *projPart = projectPart();
    if (!projPart)
        return;

    if (d->m_localRenaming.isActive()
            && d->m_localRenaming.isSameSelection(textCursor().position())) {
        return;
    }
    d->m_useSelectionsUpdater.abortSchedule();

    QPointer<CppEditorWidget> cppEditorWidget = this;

    auto renameSymbols = [=](const QString &symbolName,
                             const SourceLocationsContainer &sourceLocations,
                             int revision) {
        if (cppEditorWidget) {
            viewport()->setCursor(Qt::IBeamCursor);

            if (revision != document()->revision())
                return;
            if (sourceLocations.hasContent()) {
                QList<QTextEdit::ExtraSelection> selections
                    = sourceLocationsToExtraSelections(sourceLocations.sourceLocationContainers(),
                                                       static_cast<uint>(symbolName.size()),
                                                       cppEditorWidget);
                setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection, selections);
                d->m_localRenaming.stop();
                d->m_localRenaming.updateSelectionsForVariableUnderCursor(selections);
            }
            if (!d->m_localRenaming.start())
                cppEditorWidget->renameUsages();
        }
    };

    viewport()->setCursor(Qt::BusyCursor);
    d->m_modelManager->startLocalRenaming(CursorInEditor{textCursor(),
                                                         textDocument()->filePath(),
                                                         this, textDocument()},
                                          projPart,
                                          std::move(renameSymbols));
}

void CppEditorWidget::updatePreprocessorButtonTooltip()
{
    if (!d->m_preprocessorButton)
        return;

    Command *cmd = ActionManager::command(Constants::OPEN_PREPROCESSOR_DIALOG);
    QTC_ASSERT(cmd, return );
    d->m_preprocessorButton->setToolTip(cmd->action()->toolTip());
}

void CppEditorWidget::switchDeclarationDefinition(bool inNextSplit)
{
    if (!d->m_modelManager)
        return;

    const CursorInEditor cursor(textCursor(), textDocument()->filePath(), this, textDocument());
    auto callback = [self = QPointer(this),
            split = inNextSplit != alwaysOpenLinksInNextSplit()](const Link &link) {
        if (self && link.hasValidTarget())
            self->openLink(link, split);
    };
    followSymbolInterface().switchDeclDef(cursor, std::move(callback),
                                          d->m_modelManager->snapshot(), d->m_lastSemanticInfo.doc,
                                          d->m_modelManager->symbolFinder());
}

void CppEditorWidget::findLinkAt(const QTextCursor &cursor,
                                 Utils::ProcessLinkCallback &&processLinkCallback,
                                 bool resolveTarget,
                                 bool inNextSplit)
{
    if (!d->m_modelManager)
        return processLinkCallback(Utils::Link());

    const Utils::FilePath &filePath = textDocument()->filePath();

    followSymbolInterface().findLink(
                CursorInEditor{cursor, filePath, this, textDocument()},
                std::move(processLinkCallback),
                resolveTarget,
                d->m_modelManager->snapshot(),
                d->m_lastSemanticInfo.doc,
                d->m_modelManager->symbolFinder(),
                inNextSplit);
}

unsigned CppEditorWidget::documentRevision() const
{
    return document()->revision();
}

FollowSymbolInterface &CppEditorWidget::followSymbolInterface() const
{
    return d->m_modelManager->followSymbolInterface();
}

bool CppEditorWidget::isSemanticInfoValidExceptLocalUses() const
{
    return d->m_lastSemanticInfo.doc && d->m_lastSemanticInfo.revision == documentRevision()
           && !d->m_lastSemanticInfo.snapshot.isEmpty();
}

bool CppEditorWidget::isSemanticInfoValid() const
{
    return isSemanticInfoValidExceptLocalUses() && d->m_lastSemanticInfo.localUsesUpdated;
}

bool CppEditorWidget::isRenaming() const
{
    return d->m_localRenaming.isActive();
}

SemanticInfo CppEditorWidget::semanticInfo() const
{
    return d->m_lastSemanticInfo;
}

bool CppEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        // handle escape manually if a rename is active
        if (static_cast<QKeyEvent *>(e)->key() == Qt::Key_Escape && d->m_localRenaming.isActive()) {
            e->accept();
            return true;
        }
        break;
    default:
        break;
    }

    return TextEditorWidget::event(e);
}

void CppEditorWidget::processKeyNormally(QKeyEvent *e)
{
    TextEditorWidget::keyPressEvent(e);
}

static void addRefactoringActions(QMenu *menu, AssistInterface *iface)
{
    if (!iface || !menu)
        return;

    using Processor = QScopedPointer<IAssistProcessor>;
    using Proposal = QScopedPointer<IAssistProposal>;

    const Processor processor(CppEditorPlugin::instance()->quickFixProvider()->createProcessor());
    const Proposal proposal(processor->perform(iface)); // OK, perform() takes ownership of iface.
    if (proposal) {
        auto model = proposal->model().staticCast<GenericProposalModel>();
        for (int index = 0; index < model->size(); ++index) {
            const auto item = static_cast<AssistProposalItem *>(model->proposalItem(index));
            const QuickFixOperation::Ptr op = item->data().value<QuickFixOperation::Ptr>();
            const QAction *action = menu->addAction(op->description());
            QObject::connect(action, &QAction::triggered, menu, [op] { op->perform(); });
        }
    }
}

class ProgressIndicatorMenuItem : public QWidgetAction
{
    Q_OBJECT

public:
    ProgressIndicatorMenuItem(QObject *parent) : QWidgetAction(parent) {}

protected:
    QWidget *createWidget(QWidget *parent = nullptr) override
    {
        return new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small, parent);
    }
};

QMenu *CppEditorWidget::createRefactorMenu(QWidget *parent) const
{
    auto *menu = new QMenu(tr("&Refactor"), parent);
    menu->addAction(ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action());

    // ### enable
    // updateSemanticInfo(m_semanticHighlighter->semanticInfo(currentSource()));

    if (isSemanticInfoValidExceptLocalUses()) {
        d->m_useSelectionsUpdater.abortSchedule();

        const CppUseSelectionsUpdater::RunnerInfo runnerInfo = d->m_useSelectionsUpdater.update();
        switch (runnerInfo) {
        case CppUseSelectionsUpdater::RunnerInfo::AlreadyUpToDate:
            addRefactoringActions(menu, createAssistInterface(QuickFix, ExplicitlyInvoked));
            break;
        case CppUseSelectionsUpdater::RunnerInfo::Started: {
            // Update the refactor menu once we get the results.
            auto *progressIndicatorMenuItem = new ProgressIndicatorMenuItem(menu);
            menu->addAction(progressIndicatorMenuItem);

            connect(&d->m_useSelectionsUpdater, &CppUseSelectionsUpdater::finished,
                    menu, [=] (SemanticInfo::LocalUseMap, bool success) {
                QTC_CHECK(success);
                menu->removeAction(progressIndicatorMenuItem);
                addRefactoringActions(menu, createAssistInterface(QuickFix, ExplicitlyInvoked));
            });
            break;
        }
        case CppUseSelectionsUpdater::RunnerInfo::FailedToStart:
        case CppUseSelectionsUpdater::RunnerInfo::Invalid:
            QTC_CHECK(false && "Unexpected CppUseSelectionsUpdater runner result");
        }
    }

    return menu;
}

static void appendCustomContextMenuActionsAndMenus(QMenu *menu, QMenu *refactorMenu)
{
    bool isRefactoringMenuAdded = false;
    const QMenu *contextMenu = ActionManager::actionContainer(Constants::M_CONTEXT)->menu();
    for (QAction *action : contextMenu->actions()) {
        menu->addAction(action);
        if (action->objectName() == QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT)) {
            isRefactoringMenuAdded = true;
            menu->addMenu(refactorMenu);
        }
    }

    QTC_CHECK(isRefactoringMenuAdded);
}

void CppEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    const QPointer<QMenu> menu(new QMenu(this));

    appendCustomContextMenuActionsAndMenus(menu, createRefactorMenu(menu));
    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    if (menu)
        delete menu; // OK, menu was not already deleted by closed editor widget.
}

void CppEditorWidget::keyPressEvent(QKeyEvent *e)
{
    if (d->m_localRenaming.handleKeyPressEvent(e))
        return;

    if (handleStringSplitting(e))
        return;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        if (trySplitComment(this, semanticInfo().snapshot)) {
            e->accept();
            return;
        }
    }

    TextEditorWidget::keyPressEvent(e);
}

bool CppEditorWidget::handleStringSplitting(QKeyEvent *e) const
{
    if (!TextEditorSettings::completionSettings().m_autoSplitStrings)
        return false;

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();

        const Kind stringKind = CPlusPlus::MatchingText::stringKindAtCursor(cursor);
        if (stringKind >= T_FIRST_STRING_LITERAL && stringKind < T_FIRST_RAW_STRING_LITERAL) {
            cursor.beginEditBlock();
            if (cursor.positionInBlock() > 0
                && cursor.block().text().at(cursor.positionInBlock() - 1) == QLatin1Char('\\')) {
                // Already escaped: simply go back to line, but do not indent.
                cursor.insertText(QLatin1String("\n"));
            } else if (e->modifiers() & Qt::ShiftModifier) {
                // With 'shift' modifier, escape the end of line character
                // and start at beginning of next line.
                cursor.insertText(QLatin1String("\\\n"));
            } else {
                // End the current string, and start a new one on the line, properly indented.
                cursor.insertText(QLatin1String("\"\n\""));
                textDocument()->autoIndent(cursor);
            }
            cursor.endEditBlock();
            e->accept();
            return true;
        }
    }

    return false;
}

void CppEditorWidget::slotCodeStyleSettingsChanged(const QVariant &)
{
    QtStyleCodeFormatter formatter;
    formatter.invalidateCache(document());
}

void CppEditorWidget::updateSemanticInfo()
{
    updateSemanticInfo(d->m_cppEditorDocument->recalculateSemanticInfo(),
                       /*updateUseSelectionSynchronously=*/ true);
}

void CppEditorWidget::updateSemanticInfo(const SemanticInfo &semanticInfo,
                                         bool updateUseSelectionSynchronously)
{
    if (semanticInfo.revision != documentRevision())
        return;

    d->m_lastSemanticInfo = semanticInfo;

    if (!d->m_localRenaming.isActive()) {
        const CppUseSelectionsUpdater::CallType type = updateUseSelectionSynchronously
            ? CppUseSelectionsUpdater::CallType::Synchronous
            : CppUseSelectionsUpdater::CallType::Asynchronous;
        d->m_useSelectionsUpdater.update(type);
    }

    // schedule a check for a decl/def link
    updateFunctionDeclDefLink();
}

AssistInterface *CppEditorWidget::createAssistInterface(AssistKind kind, AssistReason reason) const
{
    if (kind == Completion || kind == FunctionHint) {
        CppCompletionAssistProvider * const cap = kind == Completion
                ? qobject_cast<CppCompletionAssistProvider *>(cppEditorDocument()->completionAssistProvider())
                : qobject_cast<CppCompletionAssistProvider *>(cppEditorDocument()->functionHintAssistProvider());
        if (cap) {
            LanguageFeatures features = LanguageFeatures::defaultFeatures();
            if (Document::Ptr doc = d->m_lastSemanticInfo.doc)
                features = doc->languageFeatures();
            features.objCEnabled |= cppEditorDocument()->isObjCEnabled();
            return cap->createAssistInterface(textDocument()->filePath(),
                                              this,
                                              features,
                                              position(),
                                              reason);
        } else {
            return TextEditorWidget::createAssistInterface(kind, reason);
        }
    } else if (kind == QuickFix) {
        if (isSemanticInfoValid())
            return new CppQuickFixInterface(const_cast<CppEditorWidget *>(this), reason);
    } else {
        return TextEditorWidget::createAssistInterface(kind, reason);
    }
    return nullptr;
}

QSharedPointer<FunctionDeclDefLink> CppEditorWidget::declDefLink() const
{
    return d->m_declDefLink;
}

void CppEditorWidget::updateFunctionDeclDefLink()
{
    const int pos = textCursor().selectionStart();

    // if there's already a link, abort it if the cursor is outside or the name changed
    // (adding a prefix is an exception since the user might type a return type)
    if (d->m_declDefLink
        && (pos < d->m_declDefLink->linkSelection.selectionStart()
            || pos > d->m_declDefLink->linkSelection.selectionEnd()
            || !d->m_declDefLink->nameSelection.selectedText().trimmed().endsWith(
                   d->m_declDefLink->nameInitial))) {
        abortDeclDefLink();
        return;
    }

    // don't start a new scan if there's one active and the cursor is already in the scanned area
    const QTextCursor scannedSelection = d->m_declDefLinkFinder->scannedSelection();
    if (!scannedSelection.isNull() && scannedSelection.selectionStart() <= pos
        && scannedSelection.selectionEnd() >= pos) {
        return;
    }

    d->m_updateFunctionDeclDefLinkTimer.start();
}

void CppEditorWidget::updateFunctionDeclDefLinkNow()
{
    IEditor *editor = EditorManager::currentEditor();
    if (!editor || editor->widget() != this)
        return;

    const Snapshot semanticSnapshot = d->m_lastSemanticInfo.snapshot;
    const Document::Ptr semanticDoc = d->m_lastSemanticInfo.doc;

    if (d->m_declDefLink) {
        // update the change marker
        const Utils::ChangeSet changes = d->m_declDefLink->changes(semanticSnapshot);
        if (changes.isEmpty())
            d->m_declDefLink->hideMarker(this);
        else
            d->m_declDefLink->showMarker(this);
        return;
    }

    if (!isSemanticInfoValidExceptLocalUses())
        return;

    Snapshot snapshot = d->m_modelManager->snapshot();
    snapshot.insert(semanticDoc);

    d->m_declDefLinkFinder->startFindLinkAt(textCursor(), semanticDoc, snapshot);
}

void CppEditorWidget::onFunctionDeclDefLinkFound(QSharedPointer<FunctionDeclDefLink> link)
{
    abortDeclDefLink();
    d->m_declDefLink = link;
    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->filePath());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            connect(textDocument,
                    &IDocument::contentsChanged,
                    this,
                    &CppEditorWidget::abortDeclDefLink);
    }
}

void CppEditorWidget::applyDeclDefLinkChanges(bool jumpToMatch)
{
    if (!d->m_declDefLink)
        return;
    d->m_declDefLink->apply(this, jumpToMatch);
    abortDeclDefLink();
    updateFunctionDeclDefLink();
}

void CppEditorWidget::encourageApply()
{
    if (d->m_localRenaming.encourageApply())
        return;

    TextEditorWidget::encourageApply();
}

void CppEditorWidget::abortDeclDefLink()
{
    if (!d->m_declDefLink)
        return;

    IDocument *targetDocument = DocumentModel::documentForFilePath(
        d->m_declDefLink->targetFile->filePath());
    if (textDocument() != targetDocument) {
        if (auto textDocument = qobject_cast<BaseTextDocument *>(targetDocument))
            disconnect(textDocument,
                       &IDocument::contentsChanged,
                       this,
                       &CppEditorWidget::abortDeclDefLink);
    }

    d->m_declDefLink->hideMarker(this);
    d->m_declDefLink.clear();
}

void CppEditorWidget::showPreProcessorWidget()
{
    const QString filePath = textDocument()->filePath().toString();

    CppPreProcessorDialog dialog(filePath, this);
    if (dialog.exec() == QDialog::Accepted) {
        const QByteArray extraDirectives = dialog.extraPreprocessorDirectives().toUtf8();
        cppEditorDocument()->setExtraPreprocessorDirectives(extraDirectives);
        cppEditorDocument()->scheduleProcessDocument();
    }
}

void CppEditorWidget::invokeTextEditorWidgetAssist(TextEditor::AssistKind assistKind,
                                                   TextEditor::IAssistProvider *provider)
{
    invokeAssist(assistKind, provider);
}

const QList<QTextEdit::ExtraSelection> CppEditorWidget::unselectLeadingWhitespace(
        const QList<QTextEdit::ExtraSelection> &selections)
{
    QList<QTextEdit::ExtraSelection> filtered;
    for (const QTextEdit::ExtraSelection &sel : selections) {
        QList<QTextEdit::ExtraSelection> splitSelections;
        int firstNonWhitespacePos = -1;
        int lastNonWhitespacePos = -1;
        bool split = false;
        const QTextBlock firstBlock = sel.cursor.document()->findBlock(sel.cursor.selectionStart());
        bool inIndentation = firstBlock.position() == sel.cursor.selectionStart();
        const auto createSplitSelection = [&] {
            QTextEdit::ExtraSelection newSelection;
            newSelection.cursor = QTextCursor(sel.cursor.document());
            newSelection.cursor.setPosition(firstNonWhitespacePos);
            newSelection.cursor.setPosition(lastNonWhitespacePos + 1, QTextCursor::KeepAnchor);
            newSelection.format = sel.format;
            splitSelections << newSelection;
        };
        for (int i = sel.cursor.selectionStart(); i < sel.cursor.selectionEnd(); ++i) {
            const QChar curChar = sel.cursor.document()->characterAt(i);
            if (!curChar.isSpace()) {
                if (firstNonWhitespacePos == -1)
                    firstNonWhitespacePos = i;
                lastNonWhitespacePos = i;
            }
            if (!inIndentation) {
                if (curChar == QChar::ParagraphSeparator)
                    inIndentation = true;
                continue;
            }
            if (curChar == QChar::ParagraphSeparator)
                continue;
            if (curChar.isSpace()) {
                if (firstNonWhitespacePos != -1) {
                    createSplitSelection();
                    firstNonWhitespacePos = -1;
                    lastNonWhitespacePos = -1;
                }
                split = true;
                continue;
            }
            inIndentation = false;
        }

        if (!split) {
            filtered << sel;
            continue;
        }

        if (firstNonWhitespacePos != -1)
            createSplitSelection();
        filtered << splitSelections;
    }
    return filtered;
}

bool CppEditorWidget::isInTestMode() const { return d->inTestMode; }

#ifdef WITH_TESTS
void CppEditorWidget::enableTestMode() { d->inTestMode = true; }
#endif

} // namespace CppEditor

#include "cppeditorwidget.moc"
