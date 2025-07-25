// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbaseeditor.h"

#include "baseannotationhighlighter.h"
#include "diffandloghighlighter.h"
#include "vcsbaseeditorconfig.h"
#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcscommand.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/patchtool.h>
#include <coreplugin/vcsmanager.h>

#include <cpaster/codepasterservice.h>

#include <diffeditor/diffeditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <solutions/spinner/spinner.h>

#include <solutions/tasking/tasktreerunner.h>

#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/syntaxhighlighter.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>

/*!
    \enum VcsBase::EditorContentType

    This enum describes the contents of a VcsBaseEditor and its interaction.

    \value RegularCommandOutput  No special handling.
    \value LogOutput  Log of a file under revision control. Provide a
           description of the change that users can click to view detailed
           information about the change and \e Annotate for the log of a
           single file.
    \value AnnotateOutput  Color contents per change number and provide a
           clickable change description.
           Context menu offers annotate previous version functionality.
           Expected format:
           \code
           <change description>: file line
           \endcode
    \value DiffOutput  Diff output. Might include describe output, which consists of a
           header and diffs. Double-clicking the chunk opens the file. The context
           menu offers the functionality to revert the chunk.

    \sa VcsBase::VcsBaseEditorWidget
*/

using namespace Core;
using namespace SpinnerSolution;
using namespace Tasking;
using namespace TextEditor;
using namespace Utils;

namespace VcsBase {

/*!
    \class VcsBase::DiffChunk

    \brief The DiffChunk class provides a diff chunk consisting of file name
    and chunk data.
*/

bool DiffChunk::isValid() const
{
    return !fileName.isEmpty() && !chunk.isEmpty();
}

QByteArray DiffChunk::asPatch(const FilePath &workingDirectory) const
{
    const FilePath relativeFile = workingDirectory.isEmpty() ?
                fileName : fileName.relativeChildPath(workingDirectory);
    const QByteArray fileNameBA = QFile::encodeName(relativeFile.toUrlishString());
    QByteArray rc = "--- ";
    rc += fileNameBA;
    rc += "\n+++ ";
    rc += fileNameBA;
    rc += '\n';
    rc += chunk;
    return rc;
}

} // namespace VcsBase

namespace VcsBase {

/*!
    \class VcsBase::VcsBaseEditor

    \brief The VcsBaseEditor class implements an editor with no support for
    duplicates.

    Creates a browse combo in the toolbar for diff output.
    It also mirrors the signals of the VcsBaseEditor since the editor
    manager passes the editor around.
*/

VcsBaseEditor::VcsBaseEditor()
{
}

void VcsBaseEditor::finalizeInitialization()
{
    QTC_ASSERT(qobject_cast<VcsBaseEditorWidget *>(editorWidget()), return);
    editorWidget()->setReadOnly(true);
}

// ----------- VcsBaseEditorPrivate

namespace Internal {

/*! \class AbstractTextCursorHandler
 *  \brief The AbstractTextCursorHandler class provides an interface to handle
 *  the contents under a text cursor inside an editor.
 */
class AbstractTextCursorHandler : public QObject
{
public:
    AbstractTextCursorHandler(VcsBaseEditorWidget *editorWidget = nullptr);

    /*! Tries to find some matching contents under \a cursor.
     *
     *  It is the first function to be called because it changes the internal
     *  state of the handler. Other functions (such as
     *  highlightCurrentContents() and handleCurrentContents()) use the result
     *  of the matching.
     *
     *  Returns \c true if contents could be found.
     */
    virtual bool findContentsUnderCursor(const QTextCursor &cursor);

    //! Highlight (eg underline) the contents matched with findContentsUnderCursor()
    virtual void highlightCurrentContents() = 0;

    //! React to user-interaction with the contents matched with findContentsUnderCursor()
    virtual void handleCurrentContents() = 0;

    //! Contents matched with the last call to findContentsUnderCursor()
    virtual QString currentContents() const = 0;

    /*! Fills \a menu with contextual actions applying to the contents matched
     *  with findContentsUnderCursor().
     */
    virtual void fillContextMenu(QMenu *menu, EditorContentType type) const = 0;

    //! Editor passed on construction of this handler
    VcsBaseEditorWidget *editorWidget() const;

    //! Text cursor used to match contents with findContentsUnderCursor()
    QTextCursor currentCursor() const;

private:
    VcsBaseEditorWidget *m_editorWidget;
    QTextCursor m_currentCursor;
};

AbstractTextCursorHandler::AbstractTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : QObject(editorWidget),
      m_editorWidget(editorWidget)
{
}

bool AbstractTextCursorHandler::findContentsUnderCursor(const QTextCursor &cursor)
{
    m_currentCursor = cursor;
    return false;
}

VcsBaseEditorWidget *AbstractTextCursorHandler::editorWidget() const
{
    return m_editorWidget;
}

QTextCursor AbstractTextCursorHandler::currentCursor() const
{
    return m_currentCursor;
}

/*! \class ChangeTextCursorHandler
 *  \brief The ChangeTextCursorHandler class provides a handler for VCS change
 *  identifiers.
 */
class ChangeTextCursorHandler : public AbstractTextCursorHandler
{
    Q_OBJECT

public:
    ChangeTextCursorHandler(VcsBaseEditorWidget *editorWidget = nullptr);

    bool findContentsUnderCursor(const QTextCursor &cursor) override;
    void highlightCurrentContents() override;
    void handleCurrentContents() override;
    QString currentContents() const override;
    void fillContextMenu(QMenu *menu, EditorContentType type) const override;

private slots:
    void slotDescribe();
    void slotCopyRevision();

private:
    void addDescribeAction(QMenu *menu, const QString &change) const;
    QAction *createAnnotateAction(const QString &change, bool previous) const;
    QAction *createCopyRevisionAction(const QString &change) const;

    QString m_currentChange;
};

ChangeTextCursorHandler::ChangeTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : AbstractTextCursorHandler(editorWidget)
{
}

bool ChangeTextCursorHandler::findContentsUnderCursor(const QTextCursor &cursor)
{
    AbstractTextCursorHandler::findContentsUnderCursor(cursor);
    m_currentChange = editorWidget()->changeUnderCursor(cursor);
    return !m_currentChange.isEmpty();
}

void ChangeTextCursorHandler::highlightCurrentContents()
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = currentCursor();
    sel.cursor.select(QTextCursor::WordUnderCursor);
    sel.format.setFontUnderline(true);
    sel.format.setProperty(QTextFormat::UserProperty, m_currentChange);
    editorWidget()->setExtraSelections(VcsBaseEditorWidget::OtherSelection,
                                       QList<QTextEdit::ExtraSelection>() << sel);
}

void ChangeTextCursorHandler::handleCurrentContents()
{
    slotDescribe();
}

void ChangeTextCursorHandler::fillContextMenu(QMenu *menu, EditorContentType type) const
{
    VcsBaseEditorWidget *widget = editorWidget();
    switch (type) {
    case AnnotateOutput: { // Describe current / annotate previous
        bool currentValid = widget->isValidRevision(m_currentChange);
        menu->addSeparator();
        menu->addAction(createCopyRevisionAction(m_currentChange));
        if (currentValid)
            addDescribeAction(menu, m_currentChange);
        menu->addSeparator();
        if (currentValid)
            menu->addAction(createAnnotateAction(widget->decorateVersion(m_currentChange), false));
        const QStringList previousVersions = widget->annotationPreviousVersions(m_currentChange);
        if (!previousVersions.isEmpty()) {
            for (const QString &pv : previousVersions)
                menu->addAction(createAnnotateAction(widget->decorateVersion(pv), true));
        }
        break;
    }
    default: // Describe current / Annotate file of current
        menu->addSeparator();
        menu->addAction(createCopyRevisionAction(m_currentChange));
        addDescribeAction(menu, m_currentChange);
        if (widget->isFileLogAnnotateEnabled())
            menu->addAction(createAnnotateAction(m_currentChange, false));
        break;
    }
    widget->addChangeActions(menu, m_currentChange);
}

QString ChangeTextCursorHandler::currentContents() const
{
    return m_currentChange;
}

void ChangeTextCursorHandler::slotDescribe()
{
    emit editorWidget()->describeRequested(editorWidget()->source(), m_currentChange);
}

void ChangeTextCursorHandler::slotCopyRevision()
{
    setClipboardAndSelection(m_currentChange);
}

void ChangeTextCursorHandler::addDescribeAction(QMenu *menu, const QString &change) const
{
    auto a = new QAction(Tr::tr("&Describe Change %1").arg(change), nullptr);
    connect(a, &QAction::triggered, this, &ChangeTextCursorHandler::slotDescribe);
    menu->addAction(a);
    menu->setDefaultAction(a);
}

QAction *ChangeTextCursorHandler::createAnnotateAction(const QString &change, bool previous) const
{
    // Use 'previous' format if desired and available, else default to standard.
    const QString &format =
            previous && !editorWidget()->annotatePreviousRevisionTextFormat().isEmpty() ?
                editorWidget()->annotatePreviousRevisionTextFormat() :
                editorWidget()->annotateRevisionTextFormat();
    auto a = new QAction(format.arg(change), nullptr);
    VcsBaseEditorWidget *editor = editorWidget();
    connect(a, &QAction::triggered, editor, [editor, change] {
        editor->slotAnnotateRevision(change);
    });
    return a;
}

QAction *ChangeTextCursorHandler::createCopyRevisionAction(const QString &change) const
{
    auto a = new QAction(Tr::tr("Copy \"%1\"").arg(change), nullptr);
    a->setData(change);
    connect(a, &QAction::triggered, this, &ChangeTextCursorHandler::slotCopyRevision);
    return a;
}

/*! \class UrlTextCursorHandler
 *  \brief The UrlTextCursorHandler class provides a handler for URLs, such as
 *  http://qt-project.org/.
 *
 *  The URL pattern can be redefined in sub-classes with setUrlPattern(), by default the pattern
 *  works for hyper-text URLs.
 */
class UrlTextCursorHandler : public AbstractTextCursorHandler
{
    Q_OBJECT

public:
    UrlTextCursorHandler(VcsBaseEditorWidget *editorWidget = nullptr);

    bool findContentsUnderCursor(const QTextCursor &cursor) override;
    void highlightCurrentContents() override;
    void handleCurrentContents() override;
    void fillContextMenu(QMenu *menu, EditorContentType type) const override;
    QString currentContents() const override;

protected slots:
    virtual void slotCopyUrl();
    virtual void slotOpenUrl();

protected:
    void setUrlPattern(const QString &pattern);
    QAction *createOpenUrlAction(const QString &text) const;
    QAction *createCopyUrlAction(const QString &text) const;

private:
    class UrlData
    {
    public:
        int startColumn;
        QString url;
        qsizetype urlLength;
    };

    UrlData m_urlData;
    QRegularExpression m_pattern;
    QRegularExpression m_jiraPattern;
    QRegularExpression m_gerritPattern;
};

UrlTextCursorHandler::UrlTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : AbstractTextCursorHandler(editorWidget)
{
    setUrlPattern(QLatin1String("https?\\://[^\\s]+"));

    m_jiraPattern = QRegularExpression("(Fixes|Task-number): ([A-Z]+-[0-9]+)");
    m_gerritPattern = QRegularExpression("Change-Id: (I[a-f0-9]{40})");
}

bool UrlTextCursorHandler::findContentsUnderCursor(const QTextCursor &cursor)
{
    AbstractTextCursorHandler::findContentsUnderCursor(cursor);

    m_urlData.url.clear();
    m_urlData.startColumn = -1;
    m_urlData.urlLength = 0;

    QTextCursor cursorForUrl = cursor;
    cursorForUrl.select(QTextCursor::LineUnderCursor);
    if (cursorForUrl.hasSelection()) {
        const QString line = cursorForUrl.selectedText();
        const int cursorCol = cursor.columnNumber();

        struct {
            QRegularExpression &pattern;
            int matchNumber;
            QString urlPrefix;
        } const regexUrls[] = {
            {m_pattern, 0, ""},
            {m_jiraPattern, 2, "https://bugreports.qt.io/browse/"},
            {m_gerritPattern, 1, "https://codereview.qt-project.org/r/"},
        };
        for (const auto &r : regexUrls) {
            QRegularExpressionMatchIterator i = r.pattern.globalMatch(line);
            while (i.hasNext()) {
                const QRegularExpressionMatch match = i.next();
                const int urlMatchIndex = match.capturedStart(r.matchNumber);
                const QString url = match.captured(r.matchNumber);
                if (urlMatchIndex <= cursorCol && cursorCol <= urlMatchIndex + url.length()) {
                    m_urlData.startColumn = urlMatchIndex;
                    m_urlData.url = r.urlPrefix + url;
                    m_urlData.urlLength = url.length();
                    break;
                }
            }
        }
    }

    return m_urlData.startColumn != -1;
}

void UrlTextCursorHandler::highlightCurrentContents()
{
    const QColor linkColor = creatorColor(Theme::TextColorLink);
    QTextEdit::ExtraSelection sel;
    sel.cursor = currentCursor();
    sel.cursor.setPosition(currentCursor().position()
                           - (currentCursor().columnNumber() - m_urlData.startColumn));
    sel.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, m_urlData.urlLength);
    sel.format.setFontUnderline(true);
    sel.format.setForeground(linkColor);
    sel.format.setUnderlineColor(linkColor);
    sel.format.setProperty(QTextFormat::UserProperty, m_urlData.url);
    editorWidget()->setExtraSelections(VcsBaseEditorWidget::OtherSelection,
                                       QList<QTextEdit::ExtraSelection>() << sel);
}

void UrlTextCursorHandler::handleCurrentContents()
{
    slotOpenUrl();
}

void UrlTextCursorHandler::fillContextMenu(QMenu *menu, EditorContentType type) const
{
    Q_UNUSED(type)
    menu->addSeparator();
    menu->addAction(createOpenUrlAction(Tr::tr("Open URL in Browser...")));
    menu->addAction(createCopyUrlAction(Tr::tr("Copy URL Location")));
}

QString UrlTextCursorHandler::currentContents() const
{
    return  m_urlData.url;
}

void UrlTextCursorHandler::setUrlPattern(const QString &pattern)
{
    m_pattern = QRegularExpression(pattern);
    QTC_ASSERT(m_pattern.isValid(), return);
}

void UrlTextCursorHandler::slotCopyUrl()
{
    setClipboardAndSelection(m_urlData.url);
}

void UrlTextCursorHandler::slotOpenUrl()
{
    QDesktopServices::openUrl(QUrl(m_urlData.url));
}

QAction *UrlTextCursorHandler::createOpenUrlAction(const QString &text) const
{
    auto a = new QAction(text);
    a->setData(m_urlData.url);
    connect(a, &QAction::triggered, this, &UrlTextCursorHandler::slotOpenUrl);
    return a;
}

QAction *UrlTextCursorHandler::createCopyUrlAction(const QString &text) const
{
    auto a = new QAction(text);
    a->setData(m_urlData.url);
    connect(a, &QAction::triggered, this, &UrlTextCursorHandler::slotCopyUrl);
    return a;
}

/*! \class EmailTextCursorHandler
 *  \brief The EmailTextCursorHandler class provides a handler for email
 *  addresses.
 */
class EmailTextCursorHandler : public UrlTextCursorHandler
{
    Q_OBJECT

public:
    EmailTextCursorHandler(VcsBaseEditorWidget *editorWidget = nullptr);
    void fillContextMenu(QMenu *menu, EditorContentType type) const override;

protected slots:
    void slotOpenUrl() override;
};

EmailTextCursorHandler::EmailTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : UrlTextCursorHandler(editorWidget)
{
    setUrlPattern(QLatin1String("[a-zA-Z0-9_\\.-]+@[^@ ]+\\.[a-zA-Z]+"));
}

void EmailTextCursorHandler::fillContextMenu(QMenu *menu, EditorContentType type) const
{
    Q_UNUSED(type)
    menu->addSeparator();
    menu->addAction(createOpenUrlAction(Tr::tr("Send Email To...")));
    menu->addAction(createCopyUrlAction(Tr::tr("Copy Email Address")));
}

void EmailTextCursorHandler::slotOpenUrl()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("mailto:") + currentContents()));
}

class VcsBaseEditorWidgetPrivate
{
public:
    VcsBaseEditorWidgetPrivate(VcsBaseEditorWidget *editorWidget);

    AbstractTextCursorHandler *findTextCursorHandler(const QTextCursor &cursor);
    // creates a browse combo in the toolbar for quick access to entries.
    // Can be used for diff and log. Combo created on first call.
    QComboBox *entriesComboBox();

    TextEditorWidget *q;
    VcsBaseEditorParameters m_parameters;

    FilePath m_workingDirectory;

    QRegularExpression m_diffFilePattern;
    QRegularExpression m_logEntryPattern;
    VcsBase::Annotation m_annotation;

    QList<int> m_entrySections; // line number where this section starts
    int m_cursorLine = -1;
    int m_firstLineNumber = -1;
    int m_defaultLineNumber = -1;
    QString m_annotateRevisionTextFormat;
    QString m_annotatePreviousRevisionTextFormat;
    VcsBaseEditorConfig *m_config = nullptr;
    QList<AbstractTextCursorHandler *> m_textCursorHandlers;
    bool m_fileLogAnnotateEnabled = false;
    bool m_mouseDragging = false;

    TaskTreeRunner m_taskTreeRunner;

private:
    QComboBox *m_entriesComboBox = nullptr;
};

VcsBaseEditorWidgetPrivate::VcsBaseEditorWidgetPrivate(VcsBaseEditorWidget *editorWidget)  :
    q(editorWidget),
    m_annotateRevisionTextFormat(Tr::tr("Annotate \"%1\""))
{
    m_textCursorHandlers.append(new ChangeTextCursorHandler(editorWidget));
    m_textCursorHandlers.append(new UrlTextCursorHandler(editorWidget));
    m_textCursorHandlers.append(new EmailTextCursorHandler(editorWidget));
}

AbstractTextCursorHandler *VcsBaseEditorWidgetPrivate::findTextCursorHandler(const QTextCursor &cursor)
{
    for (AbstractTextCursorHandler *handler : std::as_const(m_textCursorHandlers)) {
        if (handler->findContentsUnderCursor(cursor))
            return handler;
    }
    return nullptr;
}

QComboBox *VcsBaseEditorWidgetPrivate::entriesComboBox()
{
    if (m_entriesComboBox)
        return m_entriesComboBox;
    m_entriesComboBox = new QComboBox;
    m_entriesComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_entriesComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_entriesComboBox->setSizePolicy(policy);

    q->insertExtraToolBarWidget(TextEditorWidget::Left, m_entriesComboBox);
    return m_entriesComboBox;
}

} // namespace Internal

/*!
    \class VcsBase::VcsBaseEditorParameters

    \brief The VcsBaseEditorParameters class is a helper class used to
    parametrize an editor with MIME type, context
    and id.

    The extension is currently only a suggestion when running
    VCS commands with redirection.

    \sa VcsBase::VcsBaseEditorWidget, VcsBase::BaseVcsEditorFactory, VcsBase::EditorContentType
*/

/*!
    \class VcsBase::VcsBaseEditorWidget

    \brief The VcsBaseEditorWidget class is the base class for editors showing
    version control system output
    of the type enumerated by EditorContentType.

    The source property should contain the file or directory the log
    refers to and will be emitted with describeRequested().
    This is for VCS that need a current directory.

    \sa VcsBase::BaseVcsEditorFactory, VcsBase::VcsBaseEditorParameters, VcsBase::EditorContentType
*/

VcsBaseEditorWidget::VcsBaseEditorWidget()
  : d(new Internal::VcsBaseEditorWidgetPrivate(this))
{
    viewport()->setMouseTracking(true);
}

void VcsBaseEditorWidget::setParameters(const VcsBaseEditorParameters &parameters)
{
    d->m_parameters = parameters;
}

static void regexpFromString(
        const QString &pattern,
        QRegularExpression *regexp,
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption)
{
    const QRegularExpression re(pattern, options);
    QTC_ASSERT(re.isValid() && re.captureCount() >= 1, return);
    *regexp = re;
}

void VcsBaseEditorWidget::setDiffFilePattern(const QString &pattern)
{
    regexpFromString(pattern, &d->m_diffFilePattern);
}

void VcsBaseEditorWidget::setLogEntryPattern(const QString &pattern)
{
    regexpFromString(pattern, &d->m_logEntryPattern);
}

void VcsBaseEditorWidget::setAnnotationEntryPattern(const QString &pattern)
{
    regexpFromString(pattern, &d->m_annotation.entryPattern, QRegularExpression::MultilineOption);
}

void VcsBaseEditorWidget::setAnnotationSeparatorPattern(const QString &pattern)
{
    regexpFromString(pattern, &d->m_annotation.separatorPattern);
}

bool VcsBaseEditorWidget::supportChangeLinks() const
{
    switch (d->m_parameters.type) {
    case LogOutput:
    case AnnotateOutput:
        return true;
    default:
        return false;
    }
}

FilePath VcsBaseEditorWidget::fileNameForLine(int line) const
{
    Q_UNUSED(line)
    return source();
}

int VcsBaseEditorWidget::firstLineNumber() const
{
    return d->m_firstLineNumber;
}

void VcsBaseEditorWidget::setFirstLineNumber(int firstLineNumber)
{
    d->m_firstLineNumber = firstLineNumber;
}

QString VcsBaseEditorWidget::lineNumber(int blockNumber) const
{
    if (d->m_firstLineNumber > 0)
        return QString::number(d->m_firstLineNumber + blockNumber);
    return TextEditorWidget::lineNumber(blockNumber);
}

int VcsBaseEditorWidget::lineNumberDigits() const
{
    if (d->m_firstLineNumber <= 0)
        return TextEditorWidget::lineNumberDigits();

    int digits = 2;
    int max = qMax(1, d->m_firstLineNumber + blockCount());
    while (max >= 100) {
        max /= 10;
        ++digits;
    }
    return digits;
}

void VcsBaseEditorWidget::finalizeInitialization()
{
    QTC_CHECK(d->m_parameters.describeFunc);
    connect(this, &VcsBaseEditorWidget::describeRequested, this, d->m_parameters.describeFunc);
    init();
}

void VcsBaseEditorWidget::init()
{
    switch (d->m_parameters.type) {
    case OtherContent:
        break;
    case LogOutput:
        connect(d->entriesComboBox(), &QComboBox::activated,
                this, &VcsBaseEditorWidget::slotJumpToEntry);
        connect(this, &PlainTextEdit::textChanged,
                this, &VcsBaseEditorWidget::slotPopulateLogBrowser);
        connect(this, &PlainTextEdit::cursorPositionChanged,
                this, &VcsBaseEditorWidget::slotCursorPositionChanged);
        break;
    case AnnotateOutput:
        // Annotation highlighting depends on contents, which is set later on
        connect(this, &PlainTextEdit::textChanged, this, &VcsBaseEditorWidget::slotActivateAnnotation);
        break;
    case DiffOutput:
        // Diff: set up diff file browsing
        connect(d->entriesComboBox(), &QComboBox::activated,
                this, &VcsBaseEditorWidget::slotJumpToEntry);
        connect(this, &PlainTextEdit::textChanged,
                this, &VcsBaseEditorWidget::slotPopulateDiffBrowser);
        connect(this, &PlainTextEdit::cursorPositionChanged,
                this, &VcsBaseEditorWidget::slotCursorPositionChanged);
        break;
    }
    if (hasDiff()) {
        setCodeFoldingSupported(true);
        textDocument()->resetSyntaxHighlighter(
            [diffFilePattern = d->m_diffFilePattern, logEntryPattern = d->m_logEntryPattern] {
                return new DiffAndLogHighlighter(diffFilePattern, logEntryPattern);
            });
    }
    // override revisions display (green or red bar on the left, marking changes):
    setRevisionsVisible(false);
}

VcsBaseEditorWidget::~VcsBaseEditorWidget()
{
    delete d;
}

void VcsBaseEditorWidget::setForceReadOnly(bool b)
{
    setReadOnly(b);
    textDocument()->setTemporary(b);
}

FilePath VcsBaseEditorWidget::source() const
{
    return VcsBase::source(textDocument());
}

void VcsBaseEditorWidget::setSource(const FilePath &source)
{
    VcsBase::setSource(textDocument(), source);
}

QString VcsBaseEditorWidget::annotateRevisionTextFormat() const
{
    return d->m_annotateRevisionTextFormat;
}

void VcsBaseEditorWidget::setAnnotateRevisionTextFormat(const QString &f)
{
    d->m_annotateRevisionTextFormat = f;
}

QString VcsBaseEditorWidget::annotatePreviousRevisionTextFormat() const
{
    return d->m_annotatePreviousRevisionTextFormat;
}

void VcsBaseEditorWidget::setAnnotatePreviousRevisionTextFormat(const QString &f)
{
    d->m_annotatePreviousRevisionTextFormat = f;
}

bool VcsBaseEditorWidget::isFileLogAnnotateEnabled() const
{
    return d->m_fileLogAnnotateEnabled;
}

void VcsBaseEditorWidget::setFileLogAnnotateEnabled(bool e)
{
    d->m_fileLogAnnotateEnabled = e;
}

void VcsBaseEditorWidget::setHighlightingEnabled(bool e)
{
    textDocument()->syntaxHighlighter()->setEnabled(e);
}

FilePath VcsBaseEditorWidget::workingDirectory() const
{
    return d->m_workingDirectory;
}

void VcsBaseEditorWidget::setWorkingDirectory(const FilePath &wd)
{
    d->m_workingDirectory = wd;
}

TextEncoding VcsBaseEditorWidget::encoding() const
{
    return textDocument()->encoding();
}

void VcsBaseEditorWidget::setEncoding(const TextEncoding &encoding)
{
    if (encoding.isValid())
        textDocument()->setEncoding(encoding);
    else
        qWarning("%s: Attempt to set invalid encoding.", Q_FUNC_INFO);
}

EditorContentType VcsBaseEditorWidget::contentType() const
{
    return d->m_parameters.type;
}

void VcsBaseEditorWidget::slotPopulateDiffBrowser()
{
    QComboBox *entriesComboBox = d->entriesComboBox();
    entriesComboBox->clear();
    d->m_entrySections.clear();
    // Create a list of section line numbers (diffed files)
    // and populate combo with filenames.
    const QTextBlock cend = document()->end();
    int lineNumber = 0;
    QString lastFileName;
    for (QTextBlock it = document()->begin(); it != cend; it = it.next(), lineNumber++) {
        const QString text = it.text();
        // Check for a new diff section (not repeating the last filename)
        if (d->m_diffFilePattern.match(text).capturedStart() == 0) {
            const QString file = fileNameFromDiffSpecification(it);
            if (!file.isEmpty() && lastFileName != file) {
                lastFileName = file;
                // ignore any headers
                d->m_entrySections.push_back(d->m_entrySections.empty() ? 0 : lineNumber);
                entriesComboBox->addItem(FilePath::fromString(file).fileName());
            }
        }
    }
}

void VcsBaseEditorWidget::slotPopulateLogBrowser()
{
    QComboBox *entriesComboBox = d->entriesComboBox();
    entriesComboBox->clear();
    d->m_entrySections.clear();
    // Create a list of section line numbers (log entries)
    // and populate combo with subjects (if any).
    const QTextBlock cend = document()->end();
    int lineNumber = 0;
    for (QTextBlock it = document()->begin(); it != cend; it = it.next(), lineNumber++) {
        const QString text = it.text();
        // Check for a new log section (not repeating the last filename)
        const QRegularExpressionMatch match = d->m_logEntryPattern.match(text);
        if (match.hasMatch()) {
            d->m_entrySections.push_back(d->m_entrySections.empty() ? 0 : lineNumber);
            QString entry = match.captured(1);
            QString subject = revisionSubject(it);
            if (!subject.isEmpty()) {
                if (subject.length() > 100) {
                    subject.truncate(97);
                    subject.append(QLatin1String("..."));
                }
                entry.append(QLatin1String(" - ")).append(subject);
            }
            entriesComboBox->addItem(entry);
        }
    }
}

void VcsBaseEditorWidget::slotJumpToEntry(int index)
{
    // goto diff/log entry as indicated by index/line number
    if (index < 0 || index >= d->m_entrySections.size())
        return;
    const int lineNumber = d->m_entrySections.at(index) + 1; // TextEdit uses 1..n convention
    // check if we need to do something, especially to avoid messing up navigation history
    int currentLine, currentColumn;
    convertPosition(position(), &currentLine, &currentColumn);
    if (lineNumber != currentLine) {
        EditorManager::addCurrentPositionToNavigationHistory();
        gotoLine(lineNumber, 0);
    }
}

// Locate a line number in the list of diff sections.
static int sectionOfLine(int line, const QList<int> &sections)
{
    const int sectionCount = sections.size();
    if (!sectionCount)
        return -1;
    // The section at s indicates where the section begins.
    for (int s = 0; s < sectionCount; s++) {
        if (line < sections.at(s))
            return s - 1;
    }
    return sectionCount - 1;
}

void VcsBaseEditorWidget::slotCursorPositionChanged()
{
    // Adapt entries combo to new position
    // if the cursor goes across a file line.
    const int newCursorLine = textCursor().blockNumber();
    if (newCursorLine != d->m_cursorLine) {
        // Which section does it belong to?
        d->m_cursorLine = newCursorLine;
        const int section = sectionOfLine(d->m_cursorLine, d->m_entrySections);
        if (section != -1) {
            QComboBox *entriesComboBox = d->entriesComboBox();
            if (entriesComboBox->currentIndex() != section) {
                QSignalBlocker blocker(entriesComboBox);
                entriesComboBox->setCurrentIndex(section);
            }
        }
    }
    TextEditorWidget::slotCursorPositionChanged();
}

void VcsBaseEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QPointer<QMenu> menu;
    // 'click on change-interaction'
    if (supportChangeLinks()) {
        const QTextCursor cursor = cursorForPosition(e->pos());
        if (Internal::AbstractTextCursorHandler *handler = d->findTextCursorHandler(cursor)) {
            menu = new QMenu;
            handler->fillContextMenu(menu, d->m_parameters.type);
        }
    }
    if (!menu) {
        menu = new QMenu;
        appendStandardContextMenuActions(menu);
    }
    switch (d->m_parameters.type) {
    case LogOutput: // log might have diff
    case DiffOutput: {
        if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
            // optional code pasting service
            menu->addSeparator();
            connect(menu->addAction(Tr::tr("Send to CodePaster...")), &QAction::triggered,
                    this, &VcsBaseEditorWidget::slotPaste);
        }
        menu->addSeparator();
        // Apply/revert diff chunk.
        const DiffChunk chunk = diffChunk(cursorForPosition(e->pos()));
        if (!canApplyDiffChunk(chunk))
            break;
        // Apply a chunk from a diff loaded into the editor. This typically will
        // not have the 'source' property set and thus will only work if the working
        // directory matches that of the patch (see findDiffFile()). In addition,
        // the user has "Open With" and choose the right diff editor so that
        // fileNameFromDiffSpecification() works.
        QAction *applyAction = menu->addAction(Tr::tr("Apply Chunk..."));
        connect(
            applyAction,
            &QAction::triggered,
            this,
            [this, chunk] { slotApplyDiffChunk(chunk, PatchAction::Apply); },
            Qt::QueuedConnection);
        // Revert a chunk from a VCS diff, which might be linked to reloading the diff.
        QAction *revertAction = menu->addAction(Tr::tr("Revert Chunk..."));
        connect(
            revertAction,
            &QAction::triggered,
            this,
            [this, chunk] { slotApplyDiffChunk(chunk, PatchAction::Revert); },
            Qt::QueuedConnection);
        // Custom diff actions
        addDiffActions(menu, chunk);
        break;
    }
    default:
        break;
    }
    connect(this, &QObject::destroyed, menu.data(), &QObject::deleteLater);
    menu->exec(e->globalPos());
    delete menu;
}

void VcsBaseEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons()) {
        d->m_mouseDragging = true;
        TextEditorWidget::mouseMoveEvent(e);
        return;
    }

    bool overrideCursor = false;
    Qt::CursorShape cursorShape;

    if (supportChangeLinks()) {
        // Link emulation behaviour for 'click on change-interaction'
        const QTextCursor cursor = cursorForPosition(e->pos());
        Internal::AbstractTextCursorHandler *handler = d->findTextCursorHandler(cursor);
        if (handler != nullptr) {
            handler->highlightCurrentContents();
            overrideCursor = true;
            cursorShape = Qt::PointingHandCursor;
        } else {
            setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>());
            overrideCursor = true;
            cursorShape = Qt::IBeamCursor;
        }
    }
    TextEditorWidget::mouseMoveEvent(e);

    if (overrideCursor)
        viewport()->setCursor(cursorShape);
}

void VcsBaseEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    const bool wasDragging = d->m_mouseDragging;
    d->m_mouseDragging = false;
    if (!wasDragging && supportChangeLinks()) {
        if (e->button() == Qt::LeftButton &&!(e->modifiers() & Qt::ShiftModifier)) {
            const QTextCursor cursor = cursorForPosition(e->pos());
            Internal::AbstractTextCursorHandler *handler = d->findTextCursorHandler(cursor);
            if (handler != nullptr) {
                handler->handleCurrentContents();
                e->accept();
                return;
            }
        }
    }
    TextEditorWidget::mouseReleaseEvent(e);
}

void VcsBaseEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (hasDiff() && e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cursor = cursorForPosition(e->pos());
        jumpToChangeFromDiff(cursor);
    }
    TextEditorWidget::mouseDoubleClickEvent(e);
}

void VcsBaseEditorWidget::keyPressEvent(QKeyEvent *e)
{
    // Do not intercept return in editable patches.
    if (hasDiff() && isReadOnly() && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) {
        jumpToChangeFromDiff(textCursor());
        return;
    }
    TextEditorWidget::keyPressEvent(e);
}

void VcsBaseEditorWidget::slotActivateAnnotation()
{
    // The annotation highlighting depends on contents (change number
    // set with assigned colors)
    if (d->m_parameters.type != AnnotateOutput)
        return;

    const QSet<QString> changes = annotationChanges();
    if (changes.isEmpty())
        return;

    disconnect(this, &PlainTextEdit::textChanged, this, &VcsBaseEditorWidget::slotActivateAnnotation);

    if (SyntaxHighlighter *ah = textDocument()->syntaxHighlighter()) {
        ah->rehighlight();
    } else {
        BaseAnnotationHighlighterCreator creator = annotationHighlighterCreator();
        textDocument()->resetSyntaxHighlighter(
            [creator, annotation = d->m_annotation] { return creator(annotation); });
    }
}

// Check for a chunk of
//       - changes          :  "@@ -91,7 +95,7 @@"
//       - merged conflicts : "@@@ -91,7 +95,7 @@@"
// and return the modified line number (here 95).
// Note that git appends stuff after "  @@"/" @@@" (function names, etc.).
static inline bool checkChunkLine(const QString &line, int *modifiedLineNumber, int numberOfAts)
{
    const QString ats(numberOfAts, QLatin1Char('@'));
    if (!line.startsWith(ats + QLatin1Char(' ')))
        return false;
    const int len = ats.size() + 1;
    const int endPos = line.indexOf(QLatin1Char(' ') + ats, len);
    if (endPos == -1)
        return false;
    // the first chunk range applies to the original file, the second one to
    // the modified file, the one we're interested in
    const int plusPos = line.indexOf(QLatin1Char('+'), len);
    if (plusPos == -1 || plusPos > endPos)
        return false;
    const int lineNumberPos = plusPos + 1;
    const int commaPos = line.indexOf(QLatin1Char(','), lineNumberPos);
    if (commaPos == -1 || commaPos > endPos) {
        // Git submodule appears as "@@ -1 +1 @@"
        *modifiedLineNumber = 1;
        return true;
    }
    const QString lineNumberStr = line.mid(lineNumberPos, commaPos - lineNumberPos);
    bool ok;
    *modifiedLineNumber = lineNumberStr.toInt(&ok);
    return ok;
}

static inline bool checkChunkLine(const QString &line, int *modifiedLineNumber)
{
    if (checkChunkLine(line, modifiedLineNumber, 2))
        return true;
    return checkChunkLine(line, modifiedLineNumber, 3);
}

void VcsBaseEditorWidget::jumpToChangeFromDiff(QTextCursor cursor)
{
    int chunkStart = 0;
    int lineCount = -1;
    const QChar deletionIndicator = QLatin1Char('-');
    // find nearest change hunk
    QTextBlock block = cursor.block();
    if (TextBlockUserData::foldingIndent(block) <= 1) {
        // We are in a diff header, do not jump anywhere.
        // DiffAndLogHighlighter sets the foldingIndent for us.
        return;
    }
    for ( ; block.isValid() ; block = block.previous()) {
        const QString line = block.text();
        if (checkChunkLine(line, &chunkStart)) {
            break;
        } else {
            if (!line.startsWith(deletionIndicator))
                ++lineCount;
        }
    }

    if (chunkStart == -1 || lineCount < 0 || !block.isValid())
        return;

    // find the filename in previous line, map depot name back
    block = block.previous();
    if (!block.isValid())
        return;
    const QString fileName = findDiffFile(fileNameFromDiffSpecification(block));

    const bool exists = fileName.isEmpty() ? false : QFileInfo::exists(fileName);

    if (!exists)
        return;

    IEditor *ed = EditorManager::openEditor(FilePath::fromString(fileName));
    if (auto editor = qobject_cast<BaseTextEditor *>(ed))
        editor->gotoLine(chunkStart + lineCount);
}

// cut out chunk and determine file name.
DiffChunk VcsBaseEditorWidget::diffChunk(QTextCursor cursor) const
{
    DiffChunk rc;
    QTC_ASSERT(hasDiff(), return rc);
    // Search back for start of chunk.
    QTextBlock block = cursor.block();
    if (block.isValid() && TextBlockUserData::foldingIndent(block) <= 1) {
        // We are in a diff header, not in a chunk!
        // DiffAndLogHighlighter sets the foldingIndent for us.
        return rc;
    }

    int chunkStart = 0;
    for ( ; block.isValid() ; block = block.previous()) {
        if (checkChunkLine(block.text(), &chunkStart))
            break;
    }
    if (!chunkStart || !block.isValid())
        return rc;
    QString header;
    rc.fileName = FilePath::fromString(findDiffFile(fileNameFromDiffSpecification(block, &header)));
    if (rc.fileName.isEmpty())
        return rc;
    // Concatenate chunk and convert
    QString unicode = block.text();
    if (!unicode.endsWith(QLatin1Char('\n'))) // Missing in case of hg.
        unicode.append(QLatin1Char('\n'));
    for (block = block.next() ; block.isValid() ; block = block.next()) {
        const QString line = block.text();
        if (checkChunkLine(line, &chunkStart)
                || d->m_diffFilePattern.match(line).capturedStart() == 0) {
            break;
        } else {
            unicode += line;
            unicode += QLatin1Char('\n');
        }
    }
    const TextEncoding encoding = textDocument()->encoding();
    if (encoding.isValid()) {
        rc.chunk = encoding.encode(unicode);
        rc.header = encoding.encode(header);
    } else {
        rc.chunk = unicode.toLocal8Bit();
        rc.header = header.toLocal8Bit();
    }
    return rc;
}

// Find the codec used for a file querying the editor.
static TextEncoding findFileCodec(const FilePath &source)
{
    IDocument *document = DocumentModel::documentForFilePath(source);
    if (auto textDocument = qobject_cast<BaseTextDocument *>(document))
        return textDocument->encoding();
    return {};
}

// Find the codec by checking the projects (root dir of project file)
static TextEncoding findProjectEncoding(const FilePath &dirPath)
{
    // Try to find a project under which file tree the file is.
    const auto projects = ProjectExplorer::ProjectManager::projects();
    const auto *p
        = findOrDefault(projects, equal(&ProjectExplorer::Project::projectDirectory, dirPath));
    return p ? p->editorConfiguration()->textEncoding() : TextEncoding();
}

TextEncoding VcsBaseEditor::getEncoding(const FilePath &source)
{
    if (!source.isEmpty()) {
        // Check file
        if (source.isFile())
            if (TextEncoding fc = findFileCodec(source); fc.isValid())
                return fc;
        // Find by project via directory
        if (TextEncoding pc = findProjectEncoding(source.isFile() ? source.absolutePath() : source); pc.isValid())
            return pc;
    }
    return QStringConverter::System;
}

TextEncoding VcsBaseEditor::getEncoding(const FilePath &workingDirectory, const QStringList &files)
{
    if (files.empty())
        return getEncoding(workingDirectory);
    return getEncoding(workingDirectory / files.front());
}

VcsBaseEditorWidget *VcsBaseEditor::getVcsBaseEditor(const IEditor *editor)
{
    if (auto be = qobject_cast<const BaseTextEditor *>(editor))
        return qobject_cast<VcsBaseEditorWidget *>(be->editorWidget());
    return nullptr;
}

// Return line number of current editor if it matches.
int VcsBaseEditor::lineNumberOfCurrentEditor(const FilePath &currentFile)
{
    IEditor *ed = EditorManager::currentEditor();
    if (!ed)
        return -1;
    if (!currentFile.isEmpty()) {
        const IDocument *idocument  = ed->document();
        if (!idocument || idocument->filePath() != currentFile)
            return -1;
    }
    auto eda = qobject_cast<const BaseTextEditor *>(ed);
    if (!eda)
        return -1;
    const int cursorLine = eda->textCursor().blockNumber() + 1;
    if (auto edw = qobject_cast<const TextEditorWidget *>(ed->widget())) {
        const int firstLine = edw->firstVisibleBlockNumber() + 1;
        const int lastLine = edw->lastVisibleBlockNumber() + 1;
        if (firstLine <= cursorLine && cursorLine < lastLine)
            return cursorLine;
        return edw->centerVisibleBlockNumber() + 1;
    }
    return cursorLine;
}

bool VcsBaseEditor::gotoLineOfEditor(IEditor *e, int lineNumber)
{
    if (lineNumber >= 0 && e) {
        if (auto be = qobject_cast<BaseTextEditor*>(e)) {
            be->gotoLine(lineNumber);
            return true;
        }
    }
    return false;
}

// Return source file or directory string depending on parameters
// ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
FilePath VcsBaseEditor::getSource(const FilePath &workingDirectory, const QString &fileName)
{
    return workingDirectory.resolvePath(fileName);
}

FilePath VcsBaseEditor::getSource(const FilePath &workingDirectory, const QStringList &fileNames)
{
    return fileNames.size() == 1
            ? getSource(workingDirectory, fileNames.front())
            : workingDirectory;
}

QString VcsBaseEditor::getTitleId(const FilePath &workingDirectory,
                                  const QStringList &fileNames,
                                  const QString &revision)
{
    QStringList nonEmptyFileNames;
    for (const QString& fileName : fileNames) {
        if (!fileName.trimmed().isEmpty())
            nonEmptyFileNames.append(fileName);
    }

    QString rc;
    switch (nonEmptyFileNames.size()) {
    case 0:
        rc = workingDirectory.toUrlishString();
        break;
    case 1:
        rc = nonEmptyFileNames.front();
        break;
    default:
        rc = nonEmptyFileNames.join(QLatin1String(", "));
        break;
    }
    if (!revision.isEmpty()) {
        rc += QLatin1Char(':');
        rc += revision;
    }
    return rc;
}

void VcsBaseEditorWidget::setEditorConfig(VcsBaseEditorConfig *config)
{
    d->m_config = config;
}

VcsBaseEditorConfig *VcsBaseEditorWidget::editorConfig() const
{
    return d->m_config;
}

void VcsBaseEditorWidget::executeTask(const ExecutableItem &task,
                                      const Storage<CommandResult> &resultStorage)
{
    if (d->m_taskTreeRunner.isRunning())
        d->m_taskTreeRunner.cancel();

    const Storage<Spinner> spinnerStorage{SpinnerSize::Large, this};

    const auto onSetup = [spinnerStorage] {
        QTimer::singleShot(100, spinnerStorage.activeStorage(), &Spinner::show);
    };
    const auto onDone = [this, resultStorage](DoneWith doneWith) {
        if (doneWith != DoneWith::Success) {
            textDocument()->setPlainText(Tr::tr("Failed to retrieve data."));
            return;
        }
        setPlainText(resultStorage->cleanedStdOut());
        gotoDefaultLine();
    };

    const Group recipe {
        spinnerStorage,
        resultStorage,
        onGroupSetup(onSetup),
        task,
        onGroupDone(onDone)
    };

    d->m_taskTreeRunner.start(recipe);
}

void VcsBaseEditorWidget::setDefaultLineNumber(int line)
{
    d->m_defaultLineNumber = line;
}

void VcsBaseEditorWidget::gotoDefaultLine()
{
    if (d->m_defaultLineNumber >= 0)
        gotoLine(d->m_defaultLineNumber);
}

void VcsBaseEditorWidget::setPlainText(const QString &text)
{
    textDocument()->setPlainText(text);
}

// Find the complete file from a diff relative specification.
QString VcsBaseEditorWidget::findDiffFile(const QString &f) const
{
    // Check if file is absolute
    const QFileInfo in(f);
    if (in.isAbsolute())
        return in.isFile() ? f : QString();

    // 1) Try base dir
    if (!d->m_workingDirectory.isEmpty()) {
        const FilePath baseFileInfo = d->m_workingDirectory.pathAppended(f);
        if (baseFileInfo.isFile())
            return baseFileInfo.absoluteFilePath().toUrlishString();
    }
    // 2) Try in source (which can be file or directory)
    const FilePath sourcePath = source();
    if (!sourcePath.isEmpty()) {
        const FilePath sourceDir = sourcePath.isDir() ? sourcePath.absoluteFilePath()
                                                      : sourcePath.absolutePath();
        const FilePath sourceFileInfo = sourceDir.pathAppended(f);
        if (sourceFileInfo.isFile())
            return sourceFileInfo.absoluteFilePath().toUrlishString();

        const FilePath topLevel =
            VcsManager::findTopLevelForDirectory(sourceDir);
        if (topLevel.isEmpty())
            return {};

        const FilePath topLevelFile = topLevel.pathAppended(f);
        if (topLevelFile.isFile())
            return topLevelFile.absoluteFilePath().toUrlishString();
    }

    // 3) Try working directory
    if (in.isFile())
        return in.absoluteFilePath();

    // 4) remove trailing tab char and try again: At least git appends \t when the
    //    filename contains spaces. Since the diff commend does use \t all of a sudden,
    //    too, when seeing spaces in a filename, I expect the same behavior in other VCS.
    if (f.endsWith(QLatin1Char('\t')))
        return findDiffFile(f.left(f.length() - 1));

    return {};
}

void VcsBaseEditorWidget::addDiffActions(QMenu *, const DiffChunk &)
{
}

void VcsBaseEditorWidget::slotAnnotateRevision(const QString &change)
{
    const int currentLine = textCursor().blockNumber() + 1;
    const FilePath fileName = fileNameForLine(currentLine).canonicalPath();
    const FilePath workingDirectory = d->m_workingDirectory.isEmpty()
            ? VcsManager::findTopLevelForDirectory(fileName.parentDir())
            : d->m_workingDirectory;
    const FilePath relativePath = fileName.isRelativePath()
            ? fileName
            : fileName.relativeChildPath(workingDirectory);
    emit annotateRevisionRequested(workingDirectory, relativePath.toUrlishString(), change, currentLine);
}

QStringList VcsBaseEditorWidget::annotationPreviousVersions(const QString &) const
{
    return {};
}

void VcsBaseEditorWidget::slotPaste()
{
    // Retrieve service by soft dependency.
    auto pasteService = ExtensionSystem::PluginManager::getObject<CodePaster::Service>();
    QTC_ASSERT(pasteService, return);
    pasteService->postCurrentEditor();
}

bool VcsBaseEditorWidget::canApplyDiffChunk(const DiffChunk &dc) const
{
    if (!dc.isValid())
        return false;
    // Default implementation using patch.exe relies on absolute paths.
    return dc.fileName.isFile() && dc.fileName.isAbsolutePath() && dc.fileName.isWritableFile();
}

// Default implementation of revert: Apply a chunk by piping it into patch,
// (passing '-R' for revert), assuming we got absolute paths from the VCS plugins.
bool VcsBaseEditorWidget::applyDiffChunk(const DiffChunk &dc, PatchAction patchAction) const
{
    return PatchTool::runPatch(dc.asPatch(d->m_workingDirectory),
                                     d->m_workingDirectory, 0, patchAction);
}

QString VcsBaseEditorWidget::fileNameFromDiffSpecification(const QTextBlock &inBlock, QString *header) const
{
    // Go back chunks
    QString fileName;
    for (QTextBlock block = inBlock; block.isValid(); block = block.previous()) {
        const QString line = block.text();
        const QRegularExpressionMatch match = d->m_diffFilePattern.match(line);
        if (match.hasMatch()) {
            const QString cap = match.captured(1);
            if (header)
                header->prepend(line + QLatin1String("\n"));
            if (fileName.isEmpty() && !cap.isEmpty())
                fileName = cap;
        } else if (!fileName.isEmpty()) {
            return findDiffFile(fileName);
        } else if (header) {
            header->clear();
        }
    }
    return fileName.isEmpty() ? QString() : findDiffFile(fileName);
}

void VcsBaseEditorWidget::addChangeActions(QMenu *, const QString &)
{
}

QSet<QString> VcsBaseEditorWidget::annotationChanges() const
{
    QSet<QString> changes;
    const QString text = toPlainText();
    QStringView txt = QStringView(text);
    if (txt.isEmpty())
        return changes;
    if (!d->m_annotation.separatorPattern.pattern().isEmpty()) {
        const QRegularExpressionMatch match = d->m_annotation.separatorPattern.match(txt);
        if (match.hasMatch())
            txt.truncate(match.capturedStart());
    }
    QRegularExpressionMatchIterator i = d->m_annotation.entryPattern.globalMatch(txt);
    while (i.hasNext()) {
        const QRegularExpressionMatch match = i.next();
        changes.insert(match.captured(1));
    }
    return changes;
}

QString VcsBaseEditorWidget::decorateVersion(const QString &revision) const
{
    return revision;
}

bool VcsBaseEditorWidget::isValidRevision(const QString &revision) const
{
    Q_UNUSED(revision)
    return true;
}

QString VcsBaseEditorWidget::revisionSubject(const QTextBlock &inBlock) const
{
    Q_UNUSED(inBlock)
    return {};
}

bool VcsBaseEditorWidget::hasDiff() const
{
    switch (d->m_parameters.type) {
    case DiffOutput:
    case LogOutput:
        return true;
    default:
        return false;
    }
}

void VcsBaseEditorWidget::slotApplyDiffChunk(const DiffChunk &chunk, PatchAction patchAction)
{
    auto textDocument = qobject_cast<TextEditor::TextDocument *>(
        DocumentModel::documentForFilePath(chunk.fileName));
    const bool isModified = textDocument && textDocument->isModified();

    if (!PatchTool::confirmPatching(this, patchAction, isModified))
        return;

    if (textDocument && !EditorManager::saveDocument(textDocument))
        return;

    if (applyDiffChunk(chunk, patchAction) && patchAction == PatchAction::Revert)
        emit diffChunkReverted();
}

// Tagging of editors for re-use.
QString VcsBaseEditor::editorTag(EditorContentType t, const FilePath &workingDirectory,
                                 const QStringList &files, const QString &revision)
{
    const QChar colon = QLatin1Char(':');
    QString rc = QString::number(t);
    rc += colon;
    if (!revision.isEmpty()) {
        rc += revision;
        rc += colon;
    }
    rc += workingDirectory.toUrlishString();
    if (!files.isEmpty()) {
        rc += colon;
        rc += files.join(QString(colon));
    }
    return rc;
}

static const char tagPropertyC[] = "_q_VcsBaseEditorTag";

void VcsBaseEditor::tagEditor(IEditor *e, const QString &tag)
{
    e->document()->setProperty(tagPropertyC, QVariant(tag));
}

IEditor *VcsBaseEditor::locateEditorByTag(const QString &tag)
{
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents) {
        const QVariant tagPropertyValue = document->property(tagPropertyC);
        if (tagPropertyValue.typeId() == QMetaType::QString && tagPropertyValue.toString() == tag)
            return DocumentModel::editorsForDocument(document).constFirst();
    }
    return nullptr;
}

/*!
    \class VcsBase::VcsEditorFactory

    \brief The VcsEditorFactory class is the base class for editor
    factories creating instances of VcsBaseEditor subclasses.

    \sa VcsBase::VcsBaseEditorWidget
*/

VcsEditorFactory::VcsEditorFactory(const VcsBaseEditorParameters &parameters)
{
    setId(parameters.id);
    setDisplayName(parameters.displayName);
    if (parameters.mimeType != DiffEditor::Constants::DIFF_EDITOR_MIMETYPE)
        addMimeType(parameters.mimeType);

    setOptionalActionMask(OptionalActions::None);
    setDuplicatedSupported(false);

    setDocumentCreator([parameters] {
        auto document = new TextDocument(parameters.id);
        document->setMimeType(parameters.mimeType);
        document->setSuspendAllowed(false);
        return document;
    });

    setEditorWidgetCreator([parameters] {
        auto widget = parameters.editorWidgetCreator();
        auto editorWidget = Aggregation::query<VcsBaseEditorWidget>(widget);
        editorWidget->setParameters(parameters);
        return widget;
    });

    setEditorCreator([] { return new VcsBaseEditor(); });
    setMarksVisible(false);
}

VcsEditorFactory::~VcsEditorFactory() = default;

} // namespace VcsBase

#ifdef WITH_TESTS
#include <QTest>

namespace VcsBase {

void VcsBaseEditorWidget::testDiffFileResolving(const VcsEditorFactory &factory)
{
    VcsBaseEditor *editor = qobject_cast<VcsBaseEditor *>(factory.createEditor());
    auto widget = qobject_cast<VcsBaseEditorWidget *>(editor->editorWidget());

    QFETCH(QByteArray, header);
    QFETCH(QByteArray, fileName);
    QTextDocument doc(QString::fromLatin1(header));
    QTextBlock block = doc.lastBlock();
    // set source root for shadow builds
    widget->setSource(FilePath::fromString(QString::fromLatin1(SRC_DIR)));
    QVERIFY(widget->fileNameFromDiffSpecification(block).endsWith(QString::fromLatin1(fileName)));

    delete editor;
}

void VcsBaseEditorWidget::testLogResolving(const VcsEditorFactory &factory,
                                           const QByteArray &data,
                                           const QByteArray &entry1,
                                           const QByteArray &entry2)
{
    VcsBaseEditor *editor = qobject_cast<VcsBaseEditor *>(factory.createEditor());
    auto widget = qobject_cast<VcsBaseEditorWidget *>(editor->editorWidget());

    widget->textDocument()->setPlainText(QLatin1String(data));
    QCOMPARE(widget->d->entriesComboBox()->itemText(0), QString::fromLatin1(entry1));
    QCOMPARE(widget->d->entriesComboBox()->itemText(1), QString::fromLatin1(entry2));

    delete editor;
}

} // VcsBase

#endif

#include "vcsbaseeditor.moc"
