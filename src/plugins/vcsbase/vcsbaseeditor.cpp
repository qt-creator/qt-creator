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

#include "vcsbaseeditor.h"
#include "diffhighlighter.h"
#include "baseannotationhighlighter.h"
#include "vcsbaseconstants.h"
#include "vcsbaseoutputwindow.h"
#include "vcsbaseplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/vcsmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/basetextdocumentlayout.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>
#include <extensionsystem/invoker.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QRegExp>
#include <QSet>
#include <QTextCodec>
#include <QTextStream>
#include <QUrl>
#include <QTextBlock>
#include <QDesktopServices>
#include <QAction>
#include <QKeyEvent>
#include <QLayout>
#include <QMenu>
#include <QTextCursor>
#include <QTextEdit>
#include <QComboBox>
#include <QToolBar>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>

/*!
    \enum VcsBase::EditorContentType

    \brief Contents of a VcsBaseEditor and its interaction.

    \value RegularCommandOutput  No special handling.
    \value LogOutput  Log of a file under revision control. Provide  'click on change'
           description and 'Annotate' if is the log of a single file.
    \value AnnotateOutput  Color contents per change number and provide 'click on change' description.
           Context menu offers "Annotate previous version". Expected format:
           \code
           <change description>: file line
           \endcode
    \value DiffOutput  Diff output. Might includes describe output, which consists of a
           header and diffs. Interaction is 'double click in  hunk' which
           opens the file. Context menu offers 'Revert chunk'.

    \sa VcsBase::VcsBaseEditorWidget
*/

namespace VcsBase {

/*!
    \class VcsBase::DiffChunk

    \brief A diff chunk consisting of file name and chunk data.
*/

bool DiffChunk::isValid() const
{
    return !fileName.isEmpty() && !chunk.isEmpty();
}

QByteArray DiffChunk::asPatch() const
{
    const QByteArray fileNameBA = QFile::encodeName(fileName);
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

    \brief An editor with no support for duplicates.

    Creates a browse combo in the toolbar for diff output.
    It also mirrors the signals of the VcsBaseEditor since the editor
    manager passes the editor around.
*/

class VcsBaseEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    VcsBaseEditor(VcsBaseEditorWidget *, const VcsBaseEditorParameters *type);

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /*parent*/) { return 0; }
    Core::Id id() const { return m_id; }

    bool isTemporary() const { return m_temporary; }
    void setTemporary(bool t) { m_temporary = t; }

signals:
    void describeRequested(const QString &source, const QString &change);
    void annotateRevisionRequested(const QString &source, const QString &change, int line);

private:
    Core::Id m_id;
    bool m_temporary;
};

VcsBaseEditor::VcsBaseEditor(VcsBaseEditorWidget *widget,
                             const VcsBaseEditorParameters *type)  :
    BaseTextEditor(widget),
    m_id(QByteArray(type->id)),
    m_temporary(false)
{
    setContext(Core::Context(type->context, TextEditor::Constants::C_TEXTEDITOR));
}

// ----------- VcsBaseEditorPrivate

namespace Internal {

/*! \class AbstractTextCursorHandler
 *  \brief Provides an interface to handle the contents under a text cursor inside an editor
 */
class AbstractTextCursorHandler : public QObject
{
public:
    AbstractTextCursorHandler(VcsBaseEditorWidget *editorWidget = 0);

    /*! \brief Try to find some matching contents under \p cursor
     *
     *  It's the first function to be called because it changes the internal state of the handler.
     *  Other functions (highlightCurrentContents(), handleCurrentContents(), ...) use the result
     *  of the matching
     *
     *  \return true If contents could be found
     */
    virtual bool findContentsUnderCursor(const QTextCursor &cursor);

    //! Highlight (eg underline) the contents matched with findContentsUnderCursor()
    virtual void highlightCurrentContents() = 0;

    //! React to user-interaction with the contents matched with findContentsUnderCursor()
    virtual void handleCurrentContents() = 0;

    //! Contents matched with the last call to findContentsUnderCursor()
    virtual QString currentContents() const = 0;

    /*! \brief Fill \p menu with contextual actions applying to the contents matched
     *         with findContentsUnderCursor()
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
 *  \brief Provides a handler for VCS change identifiers
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
    case LogOutput: { // Describe current / Annotate file of current
        menu->addSeparator();
        menu->addAction(createCopyRevisionAction(m_currentChange));
        menu->addAction(createDescribeAction(m_currentChange));
        if (widget->isFileLogAnnotateEnabled())
            menu->addAction(createAnnotateAction(m_currentChange, false));
        break;
    }
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
    default:
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
    QAction *a = new QAction(VcsBaseEditorWidget::tr("Describe Change %1").arg(change), 0);
    connect(a, SIGNAL(triggered()), this, SLOT(slotDescribe()));
    return a;
}

QAction *ChangeTextCursorHandler::createAnnotateAction(const QString &change, bool previous) const
{
    // Use 'previous' format if desired and available, else default to standard.
    const QString &format =
            previous && !editorWidget()->annotatePreviousRevisionTextFormat().isEmpty() ?
                editorWidget()->annotatePreviousRevisionTextFormat() :
                editorWidget()->annotateRevisionTextFormat();
    QAction *a = new QAction(format.arg(change), 0);
    a->setData(change);
    connect(a, SIGNAL(triggered()), editorWidget(), SLOT(slotAnnotateRevision()));
    return a;
}

QAction *ChangeTextCursorHandler::createCopyRevisionAction(const QString &change) const
{
    QAction *a = new QAction(editorWidget()->copyRevisionTextFormat().arg(change), 0);
    a->setData(change);
    connect(a, SIGNAL(triggered()), this, SLOT(slotCopyRevision()));
    return a;
}

/*! \class UrlTextCursorHandler
 *  \brief Provides a handler for URL like http://qt-project.org/
 *
 *  The URL pattern can be redefined in sub-classes with setUrlPattern(), by default the pattern
 *  works for hyper-text URL
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
    QString m_urlPattern;
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
        QRegExp urlRx(m_urlPattern);
        int urlMatchIndex = -1;
        do {
            urlMatchIndex = urlRx.indexIn(line, urlMatchIndex + 1);
            if (urlMatchIndex != -1) {
                const QString url = urlRx.cap(0);
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
    m_urlPattern = pattern;
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
    QAction *a = new QAction(text, 0);
    a->setData(m_urlData.url);
    connect(a, SIGNAL(triggered()), this, SLOT(slotOpenUrl()));
    return a;
}

QAction *UrlTextCursorHandler::createCopyUrlAction(const QString &text) const
{
    QAction *a = new QAction(text, 0);
    a->setData(m_urlData.url);
    connect(a, SIGNAL(triggered()), this, SLOT(slotCopyUrl()));
    return a;
}

/*! \class EmailTextCursorHandler
 *  \brief Provides a handler for email addresses
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
    setUrlPattern(QLatin1String("[a-zA-Z0-9_\\.]+@[a-zA-Z0-9_\\.]+"));
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
    VcsBaseEditorWidgetPrivate(VcsBaseEditorWidget* editorWidget, const VcsBaseEditorParameters *type);

    AbstractTextCursorHandler *findTextCursorHandler(const QTextCursor &cursor);
    // creates a browse combo in the toolbar for quick access to entries.
    // Can be used for diff and log. Combo created on first call.
    QComboBox *entriesComboBox();

    const VcsBaseEditorParameters *m_parameters;

    QString m_source;
    QString m_diffBaseDirectory;

    QRegExp m_diffFilePattern;
    QRegExp m_logEntryPattern;
    QList<int> m_entrySections; // line number where this section starts
    int m_cursorLine;
    QString m_annotateRevisionTextFormat;
    QString m_annotatePreviousRevisionTextFormat;
    QString m_copyRevisionTextFormat;
    bool m_fileLogAnnotateEnabled;
    TextEditor::BaseTextEditor *m_editor;
    QWidget *m_configurationWidget;
    bool m_mouseDragging;
    QList<AbstractTextCursorHandler *> m_textCursorHandlers;

    QColor m_backgroundColor;

private:
    QComboBox *m_entriesComboBox;
};

VcsBaseEditorWidgetPrivate::VcsBaseEditorWidgetPrivate(VcsBaseEditorWidget *editorWidget,
                                                       const VcsBaseEditorParameters *type)  :
    m_parameters(type),
    m_cursorLine(-1),
    m_annotateRevisionTextFormat(VcsBaseEditorWidget::tr("Annotate \"%1\"")),
    m_copyRevisionTextFormat(VcsBaseEditorWidget::tr("Copy \"%1\"")),
    m_fileLogAnnotateEnabled(false),
    m_editor(0),
    m_configurationWidget(0),
    m_mouseDragging(false),
    m_entriesComboBox(0)
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

    m_editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Left, m_entriesComboBox);
    return m_entriesComboBox;
}

} // namespace Internal

/*!
    \struct VcsBase::VcsBaseEditorParameters

    \brief Helper struct used to parametrize an editor with mime type, context
    and id. The extension is currently only a suggestion when running
    VCS commands with redirection.

    \sa VcsBase::VcsBaseEditorWidget, VcsBase::BaseVcsEditorFactory, VcsBase::EditorContentType
*/

/*!
    \class VcsBase::VcsBaseEditorWidget

    \brief Base class for editors showing version control system output
    of the type enumerated by EditorContentType.

    The source property should contain the file or directory the log
    refers to and will be emitted with describeRequested().
    This is for VCS that need a current directory.

    \sa VcsBase::BaseVcsEditorFactory, VcsBase::VcsBaseEditorParameters, VcsBase::EditorContentType
*/

VcsBaseEditorWidget::VcsBaseEditorWidget(const VcsBaseEditorParameters *type, QWidget *parent)
  : BaseTextEditorWidget(parent),
    d(new Internal::VcsBaseEditorWidgetPrivate(this, type))
{
    viewport()->setMouseTracking(true);
    setMimeType(QLatin1String(d->m_parameters->mimeType));
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

void VcsBaseEditorWidget::init()
{
    d->m_editor = editor();
    switch (d->m_parameters->type) {
    case RegularCommandOutput:
    case LogOutput:
        connect(d->entriesComboBox(), SIGNAL(activated(int)), this, SLOT(slotJumpToEntry(int)));
        connect(this, SIGNAL(textChanged()), this, SLOT(slotPopulateLogBrowser()));
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(slotCursorPositionChanged()));
        break;
    case AnnotateOutput:
        // Annotation highlighting depends on contents, which is set later on
        connect(this, SIGNAL(textChanged()), this, SLOT(slotActivateAnnotation()));
        break;
    case DiffOutput:
        // Diff: set up diff file browsing
        connect(d->entriesComboBox(), SIGNAL(activated(int)), this, SLOT(slotJumpToEntry(int)));
        connect(this, SIGNAL(textChanged()), this, SLOT(slotPopulateDiffBrowser()));
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(slotCursorPositionChanged()));
        break;
    }
    if (hasDiff()) {
        DiffHighlighter *dh = new DiffHighlighter(d->m_diffFilePattern);
        setCodeFoldingSupported(true);
        baseTextDocument()->setSyntaxHighlighter(dh);
    }
    TextEditor::TextEditorSettings::instance()->initializeEditor(this);
    // override revisions display (green or red bar on the left, marking changes):
    setRevisionsVisible(false);
}

VcsBaseEditorWidget::~VcsBaseEditorWidget()
{
    delete d;
}

void VcsBaseEditorWidget::setForceReadOnly(bool b)
{
    VcsBaseEditor *eda = qobject_cast<VcsBaseEditor *>(editor());
    QTC_ASSERT(eda != 0, return);
    setReadOnly(b);
    eda->setTemporary(b);
}

QString VcsBaseEditorWidget::source() const
{
    return d->m_source;
}

void VcsBaseEditorWidget::setSource(const  QString &source)
{
    d->m_source = source;
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

QString VcsBaseEditorWidget::diffBaseDirectory() const
{
    return d->m_diffBaseDirectory;
}

void VcsBaseEditorWidget::setDiffBaseDirectory(const QString &bd)
{
    d->m_diffBaseDirectory = bd;
}

QTextCodec *VcsBaseEditorWidget::codec() const
{
    return const_cast<QTextCodec *>(baseTextDocument()->codec());
}

void VcsBaseEditorWidget::setCodec(QTextCodec *c)
{
    if (c)
        baseTextDocument()->setCodec(c);
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

TextEditor::BaseTextEditor *VcsBaseEditorWidget::createEditor()
{
    TextEditor::BaseTextEditor *editor = new VcsBaseEditor(this, d->m_parameters);

    // Pass on signals.
    connect(this, SIGNAL(describeRequested(QString,QString)),
            editor, SIGNAL(describeRequested(QString,QString)));
    connect(this, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            editor, SIGNAL(annotateRevisionRequested(QString,QString,int)));
    return editor;
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
                entriesComboBox->addItem(QFileInfo(file).fileName());
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
        Core::EditorManager *editorManager = Core::EditorManager::instance();
        editorManager->addCurrentPositionToNavigationHistory();
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
            const bool blocked = entriesComboBox->blockSignals(true);
            entriesComboBox->setCurrentIndex(section);
            entriesComboBox->blockSignals(blocked);
        }
    }
}

void VcsBaseEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();
    // 'click on change-interaction'
    switch (d->m_parameters->type) {
    case LogOutput:
    case AnnotateOutput: {
        const QTextCursor cursor = cursorForPosition(e->pos());
        Internal::AbstractTextCursorHandler *handler = d->findTextCursorHandler(cursor);
        if (handler != 0)
            handler->fillContextMenu(menu, d->m_parameters->type);
        // Fall-through for log (might have diff)
        if (d->m_parameters->type != LogOutput)
            break;
    }
    case DiffOutput: {
        menu->addSeparator();
        connect(menu->addAction(tr("Send to CodePaster...")), SIGNAL(triggered()),
                this, SLOT(slotPaste()));
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
        connect(applyAction, SIGNAL(triggered()), this, SLOT(slotApplyDiffChunk()));
        // Revert a chunk from a VCS diff, which might be linked to reloading the diff.
        QAction *revertAction = menu->addAction(tr("Revert Chunk..."));
        revertAction->setData(qVariantFromValue(Internal::DiffChunkAction(chunk, true)));
        connect(revertAction, SIGNAL(triggered()), this, SLOT(slotApplyDiffChunk()));
        break;
    }
    default:
        break;
    }
    menu->exec(e->globalPos());
    delete menu;
}

void VcsBaseEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons()) {
        d->m_mouseDragging = true;
        TextEditor::BaseTextEditorWidget::mouseMoveEvent(e);
        return;
    }

    bool overrideCursor = false;
    Qt::CursorShape cursorShape;

    if (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput) {
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
    TextEditor::BaseTextEditorWidget::mouseMoveEvent(e);

    if (overrideCursor)
        viewport()->setCursor(cursorShape);
}

void VcsBaseEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    const bool wasDragging = d->m_mouseDragging;
    d->m_mouseDragging = false;
    if (!wasDragging && (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput)) {
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
    TextEditor::BaseTextEditorWidget::mouseReleaseEvent(e);
}

void VcsBaseEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (hasDiff() && e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        QTextCursor cursor = cursorForPosition(e->pos());
        jumpToChangeFromDiff(cursor);
    }
    TextEditor::BaseTextEditorWidget::mouseDoubleClickEvent(e);
}

void VcsBaseEditorWidget::keyPressEvent(QKeyEvent *e)
{
    // Do not intercept return in editable patches.
    if (hasDiff() && isReadOnly() && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) {
        jumpToChangeFromDiff(textCursor());
        return;
    }
    BaseTextEditorWidget::keyPressEvent(e);
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

    disconnect(this, SIGNAL(textChanged()), this, SLOT(slotActivateAnnotation()));

    if (BaseAnnotationHighlighter *ah = qobject_cast<BaseAnnotationHighlighter *>(baseTextDocument()->syntaxHighlighter())) {
        ah->setChangeNumbers(changes);
        ah->rehighlight();
    } else {
        baseTextDocument()->setSyntaxHighlighter(createAnnotationHighlighter(changes, d->m_backgroundColor));
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
    // the modified file, the one we're interested int
    const int plusPos = line.indexOf(QLatin1Char('+'), len);
    if (plusPos == -1 || plusPos > endPos)
        return false;
    const int lineNumberPos = plusPos + 1;
    const int commaPos = line.indexOf(QLatin1Char(','), lineNumberPos);
    if (commaPos == -1 || commaPos > endPos)
        return false;
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
    if (TextEditor::BaseTextDocumentLayout::foldingIndent(block) <= 1)
        /* We are in a diff header, do not jump anywhere. DiffHighlighter sets the foldingIndent for us. */
        return;
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

    Core::IEditor *ed = Core::EditorManager::openEditor(fileName, Core::Id(), Core::EditorManager::ModeSwitch);
    if (TextEditor::ITextEditor *editor = qobject_cast<TextEditor::ITextEditor *>(ed))
        editor->gotoLine(chunkStart + lineCount);
}

// cut out chunk and determine file name.
DiffChunk VcsBaseEditorWidget::diffChunk(QTextCursor cursor) const
{
    DiffChunk rc;
    QTC_ASSERT(hasDiff(), return rc);
    // Search back for start of chunk.
    QTextBlock block = cursor.block();
    if (block.isValid() && TextEditor::BaseTextDocumentLayout::foldingIndent(block) <= 1)
        /* We are in a diff header, not in a chunk! DiffHighlighter sets the foldingIndent for us. */
        return rc;

    int chunkStart = 0;
    for ( ; block.isValid() ; block = block.previous()) {
        if (checkChunkLine(block.text(), &chunkStart))
            break;
    }
    if (!chunkStart || !block.isValid())
        return rc;
    rc.fileName = findDiffFile(fileNameFromDiffSpecification(block));
    if (rc.fileName.isEmpty())
        return rc;
    // Concatenate chunk and convert
    QString unicode = block.text();
    if (!unicode.endsWith(QLatin1Char('\n'))) // Missing in case of hg.
        unicode.append(QLatin1Char('\n'));
    for (block = block.next() ; block.isValid() ; block = block.next()) {
        const QString line = block.text();
        if (checkChunkLine(line, &chunkStart)) {
            break;
        } else {
            unicode += line;
            unicode += QLatin1Char('\n');
        }
    }
    const QTextCodec *cd = textCodec();
    rc.chunk = cd ? cd->fromUnicode(unicode) : unicode.toLocal8Bit();
    return rc;
}

void VcsBaseEditorWidget::setPlainTextData(const QByteArray &data)
{
    if (data.size() > Core::EditorManager::maxTextFileSize())
        setPlainText(msgTextTooLarge(data.size()));
    else
        setPlainText(codec()->toUnicode(data));
}

void VcsBaseEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    d->m_backgroundColor = fs.toTextCharFormat(TextEditor::C_TEXT)
            .brushProperty(QTextFormat::BackgroundBrush).color();

    if (d->m_parameters->type == AnnotateOutput) {
        if (BaseAnnotationHighlighter *highlighter = qobject_cast<BaseAnnotationHighlighter *>(baseTextDocument()->syntaxHighlighter())) {
            highlighter->setBackgroundColor(d->m_backgroundColor);
            highlighter->rehighlight();
        }
    } else if (hasDiff()) {
        if (DiffHighlighter *highlighter = qobject_cast<DiffHighlighter*>(baseTextDocument()->syntaxHighlighter())) {
            static QVector<TextEditor::TextStyle> categories;
            if (categories.isEmpty()) {
                categories << TextEditor::C_TEXT
                           << TextEditor::C_ADDED_LINE
                           << TextEditor::C_REMOVED_LINE
                           << TextEditor::C_DIFF_FILE
                           << TextEditor::C_DIFF_LOCATION;
            }
            highlighter->setFormats(fs.toTextCharFormats(categories));
            highlighter->rehighlight();
        }
    }
}

const VcsBaseEditorParameters *VcsBaseEditorWidget::findType(const VcsBaseEditorParameters *array,
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
    typedef QList<Core::IEditor *> EditorList;

    const EditorList editors = Core::EditorManager::instance()->editorsForFileName(source);
    if (!editors.empty()) {
        const EditorList::const_iterator ecend =  editors.constEnd();
        for (EditorList::const_iterator it = editors.constBegin(); it != ecend; ++it)
            if (const TextEditor::BaseTextEditor *be = qobject_cast<const TextEditor::BaseTextEditor *>(*it)) {
                QTextCodec *codec = be->editorWidget()->textCodec();
                return codec;
            }
    }
    return 0;
}

// Find the codec by checking the projects (root dir of project file)
static QTextCodec *findProjectCodec(const QString &dir)
{
    typedef  QList<ProjectExplorer::Project*> ProjectList;
    // Try to find a project under which file tree the file is.
    const ProjectExplorer::SessionManager *sm = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    const ProjectList projects = sm->projects();
    if (!projects.empty()) {
        const ProjectList::const_iterator pcend = projects.constEnd();
        for (ProjectList::const_iterator it = projects.constBegin(); it != pcend; ++it)
            if (const Core::IDocument *document = (*it)->document())
                if (document->fileName().startsWith(dir)) {
                    QTextCodec *codec = (*it)->editorConfiguration()->textCodec();
                    return codec;
                }
    }
    return 0;
}

QTextCodec *VcsBaseEditorWidget::getCodec(const QString &source)
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

QTextCodec *VcsBaseEditorWidget::getCodec(const QString &workingDirectory, const QStringList &files)
{
    if (files.empty())
        return getCodec(workingDirectory);
    return getCodec(workingDirectory + QLatin1Char('/') + files.front());
}

VcsBaseEditorWidget *VcsBaseEditorWidget::getVcsBaseEditor(const Core::IEditor *editor)
{
    if (const TextEditor::BaseTextEditor *be = qobject_cast<const TextEditor::BaseTextEditor *>(editor))
        return qobject_cast<VcsBaseEditorWidget *>(be->editorWidget());
    return 0;
}

// Return line number of current editor if it matches.
int VcsBaseEditorWidget::lineNumberOfCurrentEditor(const QString &currentFile)
{
    Core::IEditor *ed = Core::EditorManager::currentEditor();
    if (!ed)
        return -1;
    if (!currentFile.isEmpty()) {
        const Core::IDocument *idocument  = ed->document();
        if (!idocument || idocument->fileName() != currentFile)
            return -1;
    }
    const TextEditor::BaseTextEditor *eda = qobject_cast<const TextEditor::BaseTextEditor *>(ed);
    if (!eda)
        return -1;
    return eda->currentLine();
}

bool VcsBaseEditorWidget::gotoLineOfEditor(Core::IEditor *e, int lineNumber)
{
    if (lineNumber >= 0 && e) {
        if (TextEditor::BaseTextEditor *be = qobject_cast<TextEditor::BaseTextEditor*>(e)) {
            be->gotoLine(lineNumber);
            return true;
        }
    }
    return false;
}

// Return source file or directory string depending on parameters
// ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
QString VcsBaseEditorWidget::getSource(const QString &workingDirectory,
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

QString VcsBaseEditorWidget::getSource(const QString &workingDirectory,
                                 const QStringList &fileNames)
{
    return fileNames.size() == 1 ?
            getSource(workingDirectory, fileNames.front()) :
            workingDirectory;
}

QString VcsBaseEditorWidget::getTitleId(const QString &workingDirectory,
                                  const QStringList &fileNames,
                                  const QString &revision)
{
    QString rc;
    switch (fileNames.size()) {
    case 0:
        rc = workingDirectory;
        break;
    case 1:
        rc = fileNames.front();
        break;
    default:
        rc = fileNames.join(QLatin1String(", "));
        break;
    }
    if (!revision.isEmpty()) {
        rc += QLatin1Char(':');
        rc += revision;
    }
    return rc;
}

bool VcsBaseEditorWidget::setConfigurationWidget(QWidget *w)
{
    if (!d->m_editor || d->m_configurationWidget)
        return false;

    d->m_configurationWidget = w;
    d->m_editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Right, w);

    return true;
}

QWidget *VcsBaseEditorWidget::configurationWidget() const
{
    return d->m_configurationWidget;
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
    if (!d->m_diffBaseDirectory.isEmpty()) {
        const QFileInfo baseFileInfo(d->m_diffBaseDirectory + slash + f);
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

        QString topLevel;
        Core::VcsManager *vcsManager = Core::ICore::vcsManager();
        vcsManager->findVersionControlForDirectory(sourceDir, &topLevel); //
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

void VcsBaseEditorWidget::slotAnnotateRevision()
{
    if (const QAction *a = qobject_cast<const QAction *>(sender()))
        emit annotateRevisionRequested(source(), a->data().toString(),
                                       editor()->currentLine());
}

QStringList VcsBaseEditorWidget::annotationPreviousVersions(const QString &) const
{
    return QStringList();
}

void VcsBaseEditorWidget::slotPaste()
{
    // Retrieve service by soft dependency.
    QObject *pasteService =
            ExtensionSystem::PluginManager::getObjectByClassName(QLatin1String("CodePaster::CodePasterService"));
    if (pasteService) {
        QMetaObject::invokeMethod(pasteService, "postCurrentEditor");
    } else {
        QMessageBox::information(this, tr("Unable to Paste"),
                                 tr("Code pasting services are not available."));
    }
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
    return VcsBasePlugin::runPatch(dc.asPatch(), QString(), 0, revert);
}

QString VcsBaseEditorWidget::fileNameFromDiffSpecification(const QTextBlock &inBlock) const
{
    // Go back chunks
    for (QTextBlock block = inBlock; block.isValid(); block = block.previous()) {
        const QString line = block.text();
        if (d->m_diffFilePattern.indexIn(line) != -1) {
            QString cap = d->m_diffFilePattern.cap(1);
            if (!cap.isEmpty())
                return findDiffFile(cap);
        }
    }
    return QString();
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
QString VcsBaseEditorWidget::editorTag(EditorContentType t,
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

void VcsBaseEditorWidget::tagEditor(Core::IEditor *e, const QString &tag)
{
    e->setProperty(tagPropertyC, QVariant(tag));
}

Core::IEditor* VcsBaseEditorWidget::locateEditorByTag(const QString &tag)
{
    Core::IEditor *rc = 0;
    foreach (Core::IEditor *ed, Core::EditorManager::instance()->openedEditors()) {
        const QVariant tagPropertyValue = ed->property(tagPropertyC);
        if (tagPropertyValue.type() == QVariant::String && tagPropertyValue.toString() == tag) {
            rc = ed;
            break;
        }
    }
    return rc;
}

} // namespace VcsBase

#ifdef WITH_TESTS
#include <QTest>

void VcsBase::VcsBaseEditorWidget::testDiffFileResolving()
{
    QFETCH(QByteArray, header);
    QFETCH(QByteArray, fileName);
    QTextDocument doc(QString::fromLatin1(header));
    init();
    QTextBlock block = doc.lastBlock();
    QVERIFY(fileNameFromDiffSpecification(block).endsWith(QString::fromLatin1(fileName)));
}

void VcsBase::VcsBaseEditorWidget::testLogResolving(QByteArray &data,
                                                    const QByteArray &entry1,
                                                    const QByteArray &entry2)
{
    init();
    setPlainText(QLatin1String(data));
    QCOMPARE(d->entriesComboBox()->itemText(0), QString::fromLatin1(entry1));
    QCOMPARE(d->entriesComboBox()->itemText(1), QString::fromLatin1(entry2));
}
#endif

#include "vcsbaseeditor.moc"
