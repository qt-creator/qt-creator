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

#include "vcsbaseeditor.h"
#include "diffandloghighlighter.h"
#include "baseannotationhighlighter.h"
#include "basevcseditorfactory.h"
#include "vcsbaseplugin.h"
#include "vcsbaseeditorconfig.h"
#include "vcscommand.h"

#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/patchtool.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cpaster/codepasterservice.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QRegExp>
#include <QSet>
#include <QTextCodec>
#include <QUrl>
#include <QTextBlock>
#include <QDesktopServices>
#include <QAction>
#include <QKeyEvent>
#include <QMenu>
#include <QTextCursor>
#include <QTextEdit>
#include <QComboBox>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>

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

using namespace TextEditor;

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

QByteArray DiffChunk::asPatch(const QString &workingDirectory) const
{
    QString relativeFile = workingDirectory.isEmpty() ?
                fileName : QDir(workingDirectory).relativeFilePath(fileName);
    const QByteArray fileNameBA = QFile::encodeName(relativeFile);
    QByteArray rc = "--- ";
    rc += fileNameBA;
    rc += "\n+++ ";
    rc += fileNameBA;
    rc += '\n';
    rc += chunk;
    return rc;
}

namespace Internal {

// Data to be passed to apply/revert diff chunk actions.
class DiffChunkAction
{
public:
    DiffChunkAction(const DiffChunk &dc = DiffChunk(), bool revertIn = false) :
        chunk(dc), revert(revertIn) {}

    DiffChunk chunk;
    bool revert;
};

} // namespace Internal
} // namespace VcsBase

Q_DECLARE_METATYPE(VcsBase::Internal::DiffChunkAction)

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
    QTC_CHECK(qobject_cast<VcsBaseEditorWidget *>(editorWidget()));
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
    AbstractTextCursorHandler(VcsBaseEditorWidget *editorWidget = 0);

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
    ChangeTextCursorHandler(VcsBaseEditorWidget *editorWidget = 0);

    bool findContentsUnderCursor(const QTextCursor &cursor);
    void highlightCurrentContents();
    void handleCurrentContents();
    QString currentContents() const;
    void fillContextMenu(QMenu *menu, EditorContentType type) const;

private slots:
    void slotDescribe();
    void slotCopyRevision();

private:
    QAction *createDescribeAction(const QString &change) const;
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
            menu->addAction(createDescribeAction(m_currentChange));
        menu->addSeparator();
        if (currentValid)
            menu->addAction(createAnnotateAction(widget->decorateVersion(m_currentChange), false));
        const QStringList previousVersions = widget->annotationPreviousVersions(m_currentChange);
        if (!previousVersions.isEmpty()) {
            foreach (const QString &pv, previousVersions)
                menu->addAction(createAnnotateAction(widget->decorateVersion(pv), true));
        }
        break;
    }
    default: // Describe current / Annotate file of current
        menu->addSeparator();
        menu->addAction(createCopyRevisionAction(m_currentChange));
        menu->addAction(createDescribeAction(m_currentChange));
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
    QApplication::clipboard()->setText(m_currentChange);
}

QAction *ChangeTextCursorHandler::createDescribeAction(const QString &change) const
{
    auto a = new QAction(VcsBaseEditorWidget::tr("&Describe Change %1").arg(change), 0);
    connect(a, &QAction::triggered, this, &ChangeTextCursorHandler::slotDescribe);
    return a;
}

QAction *ChangeTextCursorHandler::createAnnotateAction(const QString &change, bool previous) const
{
    // Use 'previous' format if desired and available, else default to standard.
    const QString &format =
            previous && !editorWidget()->annotatePreviousRevisionTextFormat().isEmpty() ?
                editorWidget()->annotatePreviousRevisionTextFormat() :
                editorWidget()->annotateRevisionTextFormat();
    auto a = new QAction(format.arg(change), 0);
    a->setData(change);
    connect(a, &QAction::triggered, editorWidget(), &VcsBaseEditorWidget::slotAnnotateRevision);
    return a;
}

QAction *ChangeTextCursorHandler::createCopyRevisionAction(const QString &change) const
{
    auto a = new QAction(editorWidget()->copyRevisionTextFormat().arg(change), 0);
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
    UrlTextCursorHandler(VcsBaseEditorWidget *editorWidget = 0);

    bool findContentsUnderCursor(const QTextCursor &cursor);
    void highlightCurrentContents();
    void handleCurrentContents();
    void fillContextMenu(QMenu *menu, EditorContentType type) const;
    QString currentContents() const;

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
    };

    UrlData m_urlData;
    QRegExp m_pattern;
};

UrlTextCursorHandler::UrlTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : AbstractTextCursorHandler(editorWidget)
{
    setUrlPattern(QLatin1String("https?\\://[^\\s]+"));
}

bool UrlTextCursorHandler::findContentsUnderCursor(const QTextCursor &cursor)
{
    AbstractTextCursorHandler::findContentsUnderCursor(cursor);

    m_urlData.url.clear();
    m_urlData.startColumn = -1;

    QTextCursor cursorForUrl = cursor;
    cursorForUrl.select(QTextCursor::LineUnderCursor);
    if (cursorForUrl.hasSelection()) {
        const QString line = cursorForUrl.selectedText();
        const int cursorCol = cursor.columnNumber();
        int urlMatchIndex = -1;
        do {
            urlMatchIndex = m_pattern.indexIn(line, urlMatchIndex + 1);
            if (urlMatchIndex != -1) {
                const QString url = m_pattern.cap(0);
                if (urlMatchIndex <= cursorCol && cursorCol <= urlMatchIndex + url.length()) {
                    m_urlData.startColumn = urlMatchIndex;
                    m_urlData.url = url;
                }
            }
        } while (urlMatchIndex != -1 && m_urlData.startColumn == -1);
    }

    return m_urlData.startColumn != -1;
}

void UrlTextCursorHandler::highlightCurrentContents()
{
    QTextEdit::ExtraSelection sel;
    sel.cursor = currentCursor();
    sel.cursor.setPosition(currentCursor().position()
                           - (currentCursor().columnNumber() - m_urlData.startColumn));
    sel.cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, m_urlData.url.length());
    sel.format.setFontUnderline(true);
    sel.format.setForeground(Qt::blue);
    sel.format.setUnderlineColor(Qt::blue);
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
    Q_UNUSED(type);
    menu->addSeparator();
    menu->addAction(createOpenUrlAction(tr("Open URL in Browser...")));
    menu->addAction(createCopyUrlAction(tr("Copy URL Location")));
}

QString UrlTextCursorHandler::currentContents() const
{
    return  m_urlData.url;
}

void UrlTextCursorHandler::setUrlPattern(const QString &pattern)
{
    m_pattern = QRegExp(pattern);
    QTC_ASSERT(m_pattern.isValid(), return);
}

void UrlTextCursorHandler::slotCopyUrl()
{
    QApplication::clipboard()->setText(m_urlData.url);
}

void UrlTextCursorHandler::slotOpenUrl()
{
    QDesktopServices::openUrl(QUrl(m_urlData.url));
}

QAction *UrlTextCursorHandler::createOpenUrlAction(const QString &text) const
{
    auto a = new QAction(text, 0);
    a->setData(m_urlData.url);
    connect(a, &QAction::triggered, this, &UrlTextCursorHandler::slotOpenUrl);
    return a;
}

QAction *UrlTextCursorHandler::createCopyUrlAction(const QString &text) const
{
    auto a = new QAction(text, 0);
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
    EmailTextCursorHandler(VcsBaseEditorWidget *editorWidget = 0);
    void fillContextMenu(QMenu *menu, EditorContentType type) const;

protected slots:
    void slotOpenUrl();
};

EmailTextCursorHandler::EmailTextCursorHandler(VcsBaseEditorWidget *editorWidget)
    : UrlTextCursorHandler(editorWidget)
{
    setUrlPattern(QLatin1String("[a-zA-Z0-9_\\.-]+@[^@ ]+\\.[a-zA-Z]+"));
}

void EmailTextCursorHandler::fillContextMenu(QMenu *menu, EditorContentType type) const
{
    Q_UNUSED(type);
    menu->addSeparator();
    menu->addAction(createOpenUrlAction(tr("Send Email To...")));
    menu->addAction(createCopyUrlAction(tr("Copy Email Address")));
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
    const VcsBaseEditorParameters *m_parameters = nullptr;

    QString m_workingDirectory;

    QRegExp m_diffFilePattern;
    QRegExp m_logEntryPattern;
    QList<int> m_entrySections; // line number where this section starts
    int m_cursorLine = -1;
    int m_firstLineNumber = -1;
    QString m_annotateRevisionTextFormat;
    QString m_annotatePreviousRevisionTextFormat;
    QString m_copyRevisionTextFormat;
    VcsBaseEditorConfig *m_config = nullptr;
    QList<AbstractTextCursorHandler *> m_textCursorHandlers;
    QPointer<VcsCommand> m_command;
    VcsBaseEditorWidget::DescribeFunc m_describeFunc = nullptr;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    bool m_fileLogAnnotateEnabled = false;
    bool m_mouseDragging = false;

private:
    QComboBox *m_entriesComboBox = nullptr;
};

VcsBaseEditorWidgetPrivate::VcsBaseEditorWidgetPrivate(VcsBaseEditorWidget *editorWidget)  :
    q(editorWidget),
    m_annotateRevisionTextFormat(VcsBaseEditorWidget::tr("Annotate \"%1\"")),
    m_copyRevisionTextFormat(VcsBaseEditorWidget::tr("Copy \"%1\""))
{
    m_textCursorHandlers.append(new ChangeTextCursorHandler(editorWidget));
    m_textCursorHandlers.append(new UrlTextCursorHandler(editorWidget));
    m_textCursorHandlers.append(new EmailTextCursorHandler(editorWidget));
}

AbstractTextCursorHandler *VcsBaseEditorWidgetPrivate::findTextCursorHandler(const QTextCursor &cursor)
{
    foreach (AbstractTextCursorHandler *handler, m_textCursorHandlers) {
        if (handler->findContentsUnderCursor(cursor))
            return handler;
    }
    return 0;
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

void VcsBaseEditorWidget::setParameters(const VcsBaseEditorParameters *parameters)
{
    QTC_CHECK(d->m_parameters == 0);
    d->m_parameters = parameters;
}

void VcsBaseEditorWidget::setDiffFilePattern(const QRegExp &pattern)
{
    QTC_ASSERT(pattern.isValid() && pattern.captureCount() >= 1, return);
    d->m_diffFilePattern = pattern;
}

void VcsBaseEditorWidget::setLogEntryPattern(const QRegExp &pattern)
{
    QTC_ASSERT(pattern.isValid() && pattern.captureCount() >= 1, return);
    d->m_logEntryPattern = pattern;
}

bool VcsBaseEditorWidget::supportChangeLinks() const
{
    switch (d->m_parameters->type) {
    case LogOutput:
    case AnnotateOutput:
        return true;
    default:
        return false;
    }
}

QString VcsBaseEditorWidget::fileNameForLine(int line) const
{
    Q_UNUSED(line);
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

void VcsBaseEditorWidget::setDescribeFunc(DescribeFunc describeFunc)
{
    d->m_describeFunc = describeFunc;
}

void VcsBaseEditorWidget::finalizeInitialization()
{
    connect(this, &VcsBaseEditorWidget::describeRequested, this, d->m_describeFunc);
    init();
}

void VcsBaseEditorWidget::init()
{
    switch (d->m_parameters->type) {
    case OtherContent:
        break;
    case LogOutput:
        connect(d->entriesComboBox(), static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                this, &VcsBaseEditorWidget::slotJumpToEntry);
        connect(this, &QPlainTextEdit::textChanged,
                this, &VcsBaseEditorWidget::slotPopulateLogBrowser);
        connect(this, &QPlainTextEdit::cursorPositionChanged,
                this, &VcsBaseEditorWidget::slotCursorPositionChanged);
        break;
    case AnnotateOutput:
        // Annotation highlighting depends on contents, which is set later on
        connect(this, &QPlainTextEdit::textChanged, this, &VcsBaseEditorWidget::slotActivateAnnotation);
        break;
    case DiffOutput:
        // Diff: set up diff file browsing
        connect(d->entriesComboBox(), static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                this, &VcsBaseEditorWidget::slotJumpToEntry);
        connect(this, &QPlainTextEdit::textChanged,
                this, &VcsBaseEditorWidget::slotPopulateDiffBrowser);
        connect(this, &QPlainTextEdit::cursorPositionChanged,
                this, &VcsBaseEditorWidget::slotCursorPositionChanged);
        break;
    }
    if (hasDiff()) {
        auto dh = new DiffAndLogHighlighter(d->m_diffFilePattern, d->m_logEntryPattern);
        setCodeFoldingSupported(true);
        textDocument()->setSyntaxHighlighter(dh);
    }
    // override revisions display (green or red bar on the left, marking changes):
    setRevisionsVisible(false);
}

VcsBaseEditorWidget::~VcsBaseEditorWidget()
{
    setCommand(0); // abort all running commands
    delete d;
}

void VcsBaseEditorWidget::setForceReadOnly(bool b)
{
    setReadOnly(b);
    textDocument()->setTemporary(b);
}

QString VcsBaseEditorWidget::source() const
{
    return VcsBasePlugin::source(textDocument());
}

void VcsBaseEditorWidget::setSource(const  QString &source)
{
    VcsBasePlugin::setSource(textDocument(), source);
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

QString VcsBaseEditorWidget::copyRevisionTextFormat() const
{
    return d->m_copyRevisionTextFormat;
}

void VcsBaseEditorWidget::setCopyRevisionTextFormat(const QString &f)
{
    d->m_copyRevisionTextFormat = f;
}

bool VcsBaseEditorWidget::isFileLogAnnotateEnabled() const
{
    return d->m_fileLogAnnotateEnabled;
}

void VcsBaseEditorWidget::setFileLogAnnotateEnabled(bool e)
{
    d->m_fileLogAnnotateEnabled = e;
}

QString VcsBaseEditorWidget::workingDirectory() const
{
    return d->m_workingDirectory;
}

void VcsBaseEditorWidget::setWorkingDirectory(const QString &wd)
{
    d->m_workingDirectory = wd;
}

QTextCodec *VcsBaseEditorWidget::codec() const
{
    return const_cast<QTextCodec *>(textDocument()->codec());
}

void VcsBaseEditorWidget::setCodec(QTextCodec *c)
{
    if (c)
        textDocument()->setCodec(c);
    else
        qWarning("%s: Attempt to set 0 codec.", Q_FUNC_INFO);
}

EditorContentType VcsBaseEditorWidget::contentType() const
{
    return d->m_parameters->type;
}

bool VcsBaseEditorWidget::isModified() const
{
    return false;
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
        if (d->m_diffFilePattern.indexIn(text) == 0) {
            const QString file = fileNameFromDiffSpecification(it);
            if (!file.isEmpty() && lastFileName != file) {
                lastFileName = file;
                // ignore any headers
                d->m_entrySections.push_back(d->m_entrySections.empty() ? 0 : lineNumber);
                entriesComboBox->addItem(Utils::FileName::fromString(file).fileName());
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
        if (d->m_logEntryPattern.indexIn(text) != -1) {
            d->m_entrySections.push_back(d->m_entrySections.empty() ? 0 : lineNumber);
            QString entry = d->m_logEntryPattern.cap(1);
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
        Core::EditorManager::addCurrentPositionToNavigationHistory();
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
    if (newCursorLine == d->m_cursorLine)
        return;
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

void VcsBaseEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QPointer<QMenu> menu = createStandardContextMenu();
    // 'click on change-interaction'
    if (supportChangeLinks()) {
        const QTextCursor cursor = cursorForPosition(e->pos());
        if (Internal::AbstractTextCursorHandler *handler = d->findTextCursorHandler(cursor))
            handler->fillContextMenu(menu, d->m_parameters->type);
    }
    switch (d->m_parameters->type) {
    case LogOutput: // log might have diff
    case DiffOutput: {
        if (ExtensionSystem::PluginManager::getObject<CodePaster::Service>()) {
            // optional code pasting service
            menu->addSeparator();
            connect(menu->addAction(tr("Send to CodePaster...")), &QAction::triggered,
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
        QAction *applyAction = menu->addAction(tr("Apply Chunk..."));
        applyAction->setData(qVariantFromValue(Internal::DiffChunkAction(chunk, false)));
        connect(applyAction, &QAction::triggered, this, &VcsBaseEditorWidget::slotApplyDiffChunk);
        // Revert a chunk from a VCS diff, which might be linked to reloading the diff.
        QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
        revertAction->setData(qVariantFromValue(Internal::DiffChunkAction(chunk, true)));
        connect(revertAction, &QAction::triggered, this, &VcsBaseEditorWidget::slotApplyDiffChunk);
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
        if (handler != 0) {
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
            if (handler != 0) {
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
    if (d->m_parameters->type != AnnotateOutput)
        return;

    const QSet<QString> changes = annotationChanges();
    if (changes.isEmpty())
        return;

    disconnect(this, &QPlainTextEdit::textChanged, this, &VcsBaseEditorWidget::slotActivateAnnotation);

    if (BaseAnnotationHighlighter *ah = qobject_cast<BaseAnnotationHighlighter *>(textDocument()->syntaxHighlighter())) {
        ah->setChangeNumbers(changes);
        ah->rehighlight();
    } else {
        textDocument()->setSyntaxHighlighter(createAnnotationHighlighter(changes));
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
    if (TextDocumentLayout::foldingIndent(block) <= 1) {
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

    const bool exists = fileName.isEmpty() ? false : QFile::exists(fileName);

    if (!exists)
        return;

    Core::IEditor *ed = Core::EditorManager::openEditor(fileName);
    if (BaseTextEditor *editor = qobject_cast<BaseTextEditor *>(ed))
        editor->gotoLine(chunkStart + lineCount);
}

// cut out chunk and determine file name.
DiffChunk VcsBaseEditorWidget::diffChunk(QTextCursor cursor) const
{
    DiffChunk rc;
    QTC_ASSERT(hasDiff(), return rc);
    // Search back for start of chunk.
    QTextBlock block = cursor.block();
    if (block.isValid() && TextDocumentLayout::foldingIndent(block) <= 1) {
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
    rc.fileName = findDiffFile(fileNameFromDiffSpecification(block, &header));
    if (rc.fileName.isEmpty())
        return rc;
    // Concatenate chunk and convert
    QString unicode = block.text();
    if (!unicode.endsWith(QLatin1Char('\n'))) // Missing in case of hg.
        unicode.append(QLatin1Char('\n'));
    for (block = block.next() ; block.isValid() ; block = block.next()) {
        const QString line = block.text();
        if (checkChunkLine(line, &chunkStart) || d->m_diffFilePattern.indexIn(line) == 0) {
            break;
        } else {
            unicode += line;
            unicode += QLatin1Char('\n');
        }
    }
    const QTextCodec *cd = textDocument()->codec();
    rc.chunk = cd ? cd->fromUnicode(unicode) : unicode.toLocal8Bit();
    rc.header = cd ? cd->fromUnicode(header) : header.toLocal8Bit();
    return rc;
}

void VcsBaseEditorWidget::reportCommandFinished(bool ok, int exitCode, const QVariant &data)
{
    Q_UNUSED(exitCode);

    hideProgressIndicator();
    if (!ok) {
        textDocument()->setPlainText(tr("Failed to retrieve data."));
    } else if (data.type() == QVariant::Int) {
        const int line = data.toInt();
        if (line >= 0)
            gotoLine(line);
    }
}

const VcsBaseEditorParameters *VcsBaseEditor::findType(const VcsBaseEditorParameters *array,
                                                       int arraySize,
                                                       EditorContentType et)
{
    for (int i = 0; i < arraySize; i++)
        if (array[i].type == et)
            return array + i;
    return 0;
}

// Find the codec used for a file querying the editor.
static QTextCodec *findFileCodec(const QString &source)
{
    Core::IDocument *document = Core::DocumentModel::documentForFilePath(source);
    if (Core::BaseTextDocument *textDocument = qobject_cast<Core::BaseTextDocument *>(document))
        return const_cast<QTextCodec *>(textDocument->codec());
    return 0;
}

// Find the codec by checking the projects (root dir of project file)
static QTextCodec *findProjectCodec(const QString &dir)
{
    typedef  QList<ProjectExplorer::Project*> ProjectList;
    // Try to find a project under which file tree the file is.
    const ProjectList projects = ProjectExplorer::SessionManager::projects();
    if (!projects.empty()) {
        const ProjectList::const_iterator pcend = projects.constEnd();
        for (ProjectList::const_iterator it = projects.constBegin(); it != pcend; ++it)
            if (const Core::IDocument *document = (*it)->document())
                if (document->filePath().toString().startsWith(dir)) {
                    QTextCodec *codec = (*it)->editorConfiguration()->textCodec();
                    return codec;
                }
    }
    return nullptr;
}

QTextCodec *VcsBaseEditor::getCodec(const QString &source)
{
    if (!source.isEmpty()) {
        // Check file
        const QFileInfo sourceFi(source);
        if (sourceFi.isFile())
            if (QTextCodec *fc = findFileCodec(source))
                return fc;
        // Find by project via directory
        if (QTextCodec *pc = findProjectCodec(sourceFi.isFile() ? sourceFi.absolutePath() : source))
            return pc;
    }
    QTextCodec *sys = QTextCodec::codecForLocale();
    return sys;
}

QTextCodec *VcsBaseEditor::getCodec(const QString &workingDirectory, const QStringList &files)
{
    if (files.empty())
        return getCodec(workingDirectory);
    return getCodec(workingDirectory + QLatin1Char('/') + files.front());
}

VcsBaseEditorWidget *VcsBaseEditor::getVcsBaseEditor(const Core::IEditor *editor)
{
    if (const BaseTextEditor *be = qobject_cast<const BaseTextEditor *>(editor))
        return qobject_cast<VcsBaseEditorWidget *>(be->editorWidget());
    return 0;
}

// Return line number of current editor if it matches.
int VcsBaseEditor::lineNumberOfCurrentEditor(const QString &currentFile)
{
    Core::IEditor *ed = Core::EditorManager::currentEditor();
    if (!ed)
        return -1;
    if (!currentFile.isEmpty()) {
        const Core::IDocument *idocument  = ed->document();
        if (!idocument || idocument->filePath().toString() != currentFile)
            return -1;
    }
    const BaseTextEditor *eda = qobject_cast<const BaseTextEditor *>(ed);
    if (!eda)
        return -1;
    const int cursorLine = eda->currentLine();
    auto const edw = qobject_cast<const TextEditorWidget *>(ed->widget());
    if (edw) {
        const int firstLine = edw->firstVisibleLine();
        const int lastLine = edw->lastVisibleLine();
        if (firstLine <= cursorLine && cursorLine < lastLine)
            return cursorLine;
        return edw->centerVisibleLine();
    }
    return cursorLine;
}

bool VcsBaseEditor::gotoLineOfEditor(Core::IEditor *e, int lineNumber)
{
    if (lineNumber >= 0 && e) {
        if (BaseTextEditor *be = qobject_cast<BaseTextEditor*>(e)) {
            be->gotoLine(lineNumber);
            return true;
        }
    }
    return false;
}

// Return source file or directory string depending on parameters
// ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
QString VcsBaseEditor::getSource(const QString &workingDirectory,
                                 const QString &fileName)
{
    if (fileName.isEmpty())
        return workingDirectory;

    QString rc = workingDirectory;
    const QChar slash = QLatin1Char('/');
    if (!rc.isEmpty() && !(rc.endsWith(slash) || rc.endsWith(QLatin1Char('\\'))))
        rc += slash;
    rc += fileName;
    return rc;
}

QString VcsBaseEditor::getSource(const QString &workingDirectory,
                                 const QStringList &fileNames)
{
    return fileNames.size() == 1 ?
            getSource(workingDirectory, fileNames.front()) :
            workingDirectory;
}

QString VcsBaseEditor::getTitleId(const QString &workingDirectory,
                                  const QStringList &fileNames,
                                  const QString &revision)
{
    QStringList nonEmptyFileNames;
    foreach (const QString& fileName, fileNames) {
        if (!fileName.trimmed().isEmpty())
            nonEmptyFileNames.append(fileName);
    }

    QString rc;
    switch (nonEmptyFileNames.size()) {
    case 0:
        rc = workingDirectory;
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

void VcsBaseEditorWidget::setCommand(VcsCommand *command)
{
    if (d->m_command) {
        d->m_command->abort();
        hideProgressIndicator();
    }
    d->m_command = command;
    if (command) {
        d->m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large);
        d->m_progressIndicator->attachToWidget(this);
        connect(command, &VcsCommand::finished, this, &VcsBaseEditorWidget::reportCommandFinished);
        QTimer::singleShot(100, this, &VcsBaseEditorWidget::showProgressIndicator);
    }
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
    const QChar slash = QLatin1Char('/');
    if (!d->m_workingDirectory.isEmpty()) {
        const QFileInfo baseFileInfo(d->m_workingDirectory + slash + f);
        if (baseFileInfo.isFile())
            return baseFileInfo.absoluteFilePath();
    }
    // 2) Try in source (which can be file or directory)
    if (!source().isEmpty()) {
        const QFileInfo sourceInfo(source());
        const QString sourceDir = sourceInfo.isDir() ? sourceInfo.absoluteFilePath()
                                                     : sourceInfo.absolutePath();
        const QFileInfo sourceFileInfo(sourceDir + slash + f);
        if (sourceFileInfo.isFile())
            return sourceFileInfo.absoluteFilePath();

        const QString topLevel = Core::VcsManager::findTopLevelForDirectory(sourceDir);
        if (topLevel.isEmpty())
            return QString();

        const QFileInfo topLevelFileInfo(topLevel + slash + f);
        if (topLevelFileInfo.isFile())
            return topLevelFileInfo.absoluteFilePath();
    }

    // 3) Try working directory
    if (in.isFile())
        return in.absoluteFilePath();

    // 4) remove trailing tab char and try again: At least git appends \t when the
    //    filename contains spaces. Since the diff commend does use \t all of a sudden,
    //    too, when seeing spaces in a filename, I expect the same behavior in other VCS.
    if (f.endsWith(QLatin1Char('\t')))
        return findDiffFile(f.left(f.length() - 1));

    return QString();
}

void VcsBaseEditorWidget::addDiffActions(QMenu *, const DiffChunk &)
{
}

void VcsBaseEditorWidget::slotAnnotateRevision()
{
    if (const QAction *a = qobject_cast<const QAction *>(sender())) {
        const int currentLine = textCursor().blockNumber() + 1;
        const QString fileName = fileNameForLine(currentLine);
        QString workingDirectory = d->m_workingDirectory;
        if (workingDirectory.isEmpty())
            workingDirectory = QFileInfo(fileName).absolutePath();
        emit annotateRevisionRequested(workingDirectory,
                                       QDir(workingDirectory).relativeFilePath(fileName),
                                       a->data().toString(), currentLine);
    }
}

QStringList VcsBaseEditorWidget::annotationPreviousVersions(const QString &) const
{
    return QStringList();
}

void VcsBaseEditorWidget::slotPaste()
{
    // Retrieve service by soft dependency.
    auto pasteService = ExtensionSystem::PluginManager::getObject<CodePaster::Service>();
    QTC_ASSERT(pasteService, return);
    pasteService->postCurrentEditor();
}

void VcsBaseEditorWidget::showProgressIndicator()
{
    if (!d->m_progressIndicator) // already stopped and deleted
        return;
    d->m_progressIndicator->show();
}

void VcsBaseEditorWidget::hideProgressIndicator()
{
    delete d->m_progressIndicator;
    d->m_progressIndicator = 0;
}

bool VcsBaseEditorWidget::canApplyDiffChunk(const DiffChunk &dc) const
{
    if (!dc.isValid())
        return false;
    const QFileInfo fi(dc.fileName);
    // Default implementation using patch.exe relies on absolute paths.
    return fi.isFile() && fi.isAbsolute() && fi.isWritable();
}

// Default implementation of revert: Apply a chunk by piping it into patch,
// (passing '-R' for revert), assuming we got absolute paths from the VCS plugins.
bool VcsBaseEditorWidget::applyDiffChunk(const DiffChunk &dc, bool revert) const
{
    return Core::PatchTool::runPatch(dc.asPatch(d->m_workingDirectory),
                                   d->m_workingDirectory, 0, revert);
}

QString VcsBaseEditorWidget::fileNameFromDiffSpecification(const QTextBlock &inBlock, QString *header) const
{
    // Go back chunks
    QString fileName;
    for (QTextBlock block = inBlock; block.isValid(); block = block.previous()) {
        const QString line = block.text();
        if (d->m_diffFilePattern.indexIn(line) != -1) {
            QString cap = d->m_diffFilePattern.cap(1);
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

QString VcsBaseEditorWidget::decorateVersion(const QString &revision) const
{
    return revision;
}

bool VcsBaseEditorWidget::isValidRevision(const QString &revision) const
{
    Q_UNUSED(revision);
    return true;
}

QString VcsBaseEditorWidget::revisionSubject(const QTextBlock &inBlock) const
{
    Q_UNUSED(inBlock);
    return QString();
}

bool VcsBaseEditorWidget::hasDiff() const
{
    switch (d->m_parameters->type) {
    case DiffOutput:
    case LogOutput:
        return true;
    default:
        return false;
    }
}

void VcsBaseEditorWidget::slotApplyDiffChunk()
{
    const QAction *a = qobject_cast<QAction *>(sender());
    QTC_ASSERT(a, return);
    const Internal::DiffChunkAction chunkAction = qvariant_cast<Internal::DiffChunkAction>(a->data());
    const QString title = chunkAction.revert ? tr("Revert Chunk") : tr("Apply Chunk");
    const QString question = chunkAction.revert ?
        tr("Would you like to revert the chunk?") : tr("Would you like to apply the chunk?");
    if (QMessageBox::No == QMessageBox::question(this, title, question, QMessageBox::Yes|QMessageBox::No))
        return;

    if (applyDiffChunk(chunkAction.chunk, chunkAction.revert)) {
        if (chunkAction.revert)
            emit diffChunkReverted(chunkAction.chunk);
        else
            emit diffChunkApplied(chunkAction.chunk);
    }
}

// Tagging of editors for re-use.
QString VcsBaseEditor::editorTag(EditorContentType t,
                                 const QString &workingDirectory,
                                 const QStringList &files,
                                 const QString &revision)
{
    const QChar colon = QLatin1Char(':');
    QString rc = QString::number(t);
    rc += colon;
    if (!revision.isEmpty()) {
        rc += revision;
        rc += colon;
    }
    rc += workingDirectory;
    if (!files.isEmpty()) {
        rc += colon;
        rc += files.join(QString(colon));
    }
    return rc;
}

static const char tagPropertyC[] = "_q_VcsBaseEditorTag";

void VcsBaseEditor::tagEditor(Core::IEditor *e, const QString &tag)
{
    e->document()->setProperty(tagPropertyC, QVariant(tag));
}

Core::IEditor *VcsBaseEditor::locateEditorByTag(const QString &tag)
{
    foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments()) {
        const QVariant tagPropertyValue = document->property(tagPropertyC);
        if (tagPropertyValue.type() == QVariant::String && tagPropertyValue.toString() == tag)
            return Core::DocumentModel::editorsForDocument(document).first();
    }
    return 0;
}

} // namespace VcsBase

#ifdef WITH_TESTS
#include <QTest>

void VcsBase::VcsBaseEditorWidget::testDiffFileResolving(const char *id)
{
    VcsBaseEditor *editor = VcsBase::VcsEditorFactory::createEditorById(id);
    VcsBaseEditorWidget *widget = qobject_cast<VcsBaseEditorWidget *>(editor->editorWidget());

    QFETCH(QByteArray, header);
    QFETCH(QByteArray, fileName);
    QTextDocument doc(QString::fromLatin1(header));
    QTextBlock block = doc.lastBlock();
    // set source root for shadow builds
    widget->setSource(QString::fromLatin1(SRC_DIR));
    QVERIFY(widget->fileNameFromDiffSpecification(block).endsWith(QString::fromLatin1(fileName)));

    delete editor;
}

void VcsBase::VcsBaseEditorWidget::testLogResolving(const char *id, QByteArray &data,
                                                    const QByteArray &entry1,
                                                    const QByteArray &entry2)
{
    VcsBaseEditor *editor = VcsBase::VcsEditorFactory::createEditorById(id);
    VcsBaseEditorWidget *widget = qobject_cast<VcsBaseEditorWidget *>(editor->editorWidget());

    widget->textDocument()->setPlainText(QLatin1String(data));
    QCOMPARE(widget->d->entriesComboBox()->itemText(0), QString::fromLatin1(entry1));
    QCOMPARE(widget->d->entriesComboBox()->itemText(1), QString::fromLatin1(entry2));

    delete editor;
}
#endif

#include "vcsbaseeditor.moc"
