/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "vcsbaseeditor.h"
#include "diffhighlighter.h"
#include "baseannotationhighlighter.h"
#include "vcsbasetextdocument.h"
#include "vcsbaseconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/ifile.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtGui/QTextBlock>
#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>
#include <QtGui/QComboBox>
#include <QtGui/QToolBar>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>

namespace VCSBase {

// VCSBaseEditor: An editor with no support for duplicates.
// Creates a browse combo in the toolbar for diff output.
// It also mirrors the signals of the VCSBaseEditor since the editor
// manager passes the editor around.
class VCSBaseEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT
public:
    VCSBaseEditor(VCSBaseEditorWidget *,
                  const VCSBaseEditorParameters *type);
    Core::Context context() const;

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /*parent*/) { return 0; }
    QString id() const { return m_id; }

    bool isTemporary() const { return m_temporary; }
    void setTemporary(bool t) { m_temporary = t; }

signals:
    void describeRequested(const QString &source, const QString &change);
    void annotateRevisionRequested(const QString &source, const QString &change, int line);

private:
    QString m_id;
    Core::Context m_context;
    bool m_temporary;
};

VCSBaseEditor::VCSBaseEditor(VCSBaseEditorWidget *widget,
                             const VCSBaseEditorParameters *type)  :
    BaseTextEditor(widget),
    m_id(type->id),
    m_context(type->context, TextEditor::Constants::C_TEXTEDITOR),
    m_temporary(false)
{
}

Core::Context VCSBaseEditor::context() const
{
    return m_context;
}

// Diff editor: creates a browse combo in the toolbar for diff output.
class VCSBaseDiffEditor : public VCSBaseEditor
{
public:
    VCSBaseDiffEditor(VCSBaseEditorWidget *, const VCSBaseEditorParameters *type);

    QComboBox *diffFileBrowseComboBox() const { return m_diffFileBrowseComboBox; }

private:
    QComboBox *m_diffFileBrowseComboBox;
};

VCSBaseDiffEditor::VCSBaseDiffEditor(VCSBaseEditorWidget *w, const VCSBaseEditorParameters *type) :
    VCSBaseEditor(w, type),
    m_diffFileBrowseComboBox(new QComboBox)
{
    m_diffFileBrowseComboBox->setMinimumContentsLength(20);
    // Make the combo box prefer to expand
    QSizePolicy policy = m_diffFileBrowseComboBox->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_diffFileBrowseComboBox->setSizePolicy(policy);

    insertExtraToolBarWidget(Left, m_diffFileBrowseComboBox);
}

// ----------- VCSBaseEditorPrivate

struct VCSBaseEditorWidgetPrivate
{
    VCSBaseEditorWidgetPrivate(const VCSBaseEditorParameters *type);

    const VCSBaseEditorParameters *m_parameters;

    QString m_currentChange;
    QString m_source;
    QString m_diffBaseDirectory;

    QRegExp m_diffFilePattern;
    QList<int> m_diffSections; // line number where this section starts
    int m_cursorLine;
    QString m_annotateRevisionTextFormat;
    QString m_annotatePreviousRevisionTextFormat;
    QString m_copyRevisionTextFormat;
    bool m_fileLogAnnotateEnabled;
    TextEditor::BaseTextEditor *m_editor;
    QWidget *m_configurationWidget;
};

VCSBaseEditorWidgetPrivate::VCSBaseEditorWidgetPrivate(const VCSBaseEditorParameters *type)  :
    m_parameters(type),
    m_cursorLine(-1),
    m_annotateRevisionTextFormat(VCSBaseEditorWidget::tr("Annotate \"%1\"")),
    m_copyRevisionTextFormat(VCSBaseEditorWidget::tr("Copy \"%1\"")),
    m_fileLogAnnotateEnabled(false),
    m_editor(0),
    m_configurationWidget(0)
{
}

// ------------ VCSBaseEditor
VCSBaseEditorWidget::VCSBaseEditorWidget(const VCSBaseEditorParameters *type, QWidget *parent)
  : BaseTextEditorWidget(parent),
    d(new VCSBaseEditorWidgetPrivate(type))
{
    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::VCSBaseEditor" << type->type << type->id;

    viewport()->setMouseTracking(true);
    setBaseTextDocument(new Internal::VCSBaseTextDocument);
    setMimeType(QLatin1String(d->m_parameters->mimeType));
}

void VCSBaseEditorWidget::init()
{
    switch (d->m_parameters->type) {
    case RegularCommandOutput:
    case LogOutput:
    case AnnotateOutput:
        // Annotation highlighting depends on contents, which is set later on
        connect(this, SIGNAL(textChanged()), this, SLOT(slotActivateAnnotation()));
        break;
    case DiffOutput: {
        DiffHighlighter *dh = createDiffHighlighter();
        setCodeFoldingSupported(true);
        baseTextDocument()->setSyntaxHighlighter(dh);
        d->m_diffFilePattern = dh->filePattern();
        connect(this, SIGNAL(textChanged()), this, SLOT(slotPopulateDiffBrowser()));
        connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(slotDiffCursorPositionChanged()));
    }
        break;
    }
}

VCSBaseEditorWidget::~VCSBaseEditorWidget()
{
    delete d;
}

void VCSBaseEditorWidget::setForceReadOnly(bool b)
{
    Internal::VCSBaseTextDocument *vbd = qobject_cast<Internal::VCSBaseTextDocument*>(baseTextDocument());
    VCSBaseEditor *eda = qobject_cast<VCSBaseEditor *>(editor());
    QTC_ASSERT(vbd != 0 && eda != 0, return);
    setReadOnly(b);
    vbd->setForceReadOnly(b);
    eda->setTemporary(b);
}

bool VCSBaseEditorWidget::isForceReadOnly() const
{
    const Internal::VCSBaseTextDocument *vbd = qobject_cast<const Internal::VCSBaseTextDocument*>(baseTextDocument());
    QTC_ASSERT(vbd, return false);
    return vbd->isForceReadOnly();
}

QString VCSBaseEditorWidget::source() const
{
    return d->m_source;
}

void VCSBaseEditorWidget::setSource(const  QString &source)
{
    d->m_source = source;
}

QString VCSBaseEditorWidget::annotateRevisionTextFormat() const
{
    return d->m_annotateRevisionTextFormat;
}

void VCSBaseEditorWidget::setAnnotateRevisionTextFormat(const QString &f)
{
    d->m_annotateRevisionTextFormat = f;
}

QString VCSBaseEditorWidget::annotatePreviousRevisionTextFormat() const
{
    return d->m_annotatePreviousRevisionTextFormat;
}

void VCSBaseEditorWidget::setAnnotatePreviousRevisionTextFormat(const QString &f)
{
    d->m_annotatePreviousRevisionTextFormat = f;
}

QString VCSBaseEditorWidget::copyRevisionTextFormat() const
{
    return d->m_copyRevisionTextFormat;
}

void VCSBaseEditorWidget::setCopyRevisionTextFormat(const QString &f)
{
    d->m_copyRevisionTextFormat = f;
}

bool VCSBaseEditorWidget::isFileLogAnnotateEnabled() const
{
    return d->m_fileLogAnnotateEnabled;
}

void VCSBaseEditorWidget::setFileLogAnnotateEnabled(bool e)
{
    d->m_fileLogAnnotateEnabled = e;
}

QString VCSBaseEditorWidget::diffBaseDirectory() const
{
    return d->m_diffBaseDirectory;
}

void VCSBaseEditorWidget::setDiffBaseDirectory(const QString &bd)
{
    d->m_diffBaseDirectory = bd;
}

QTextCodec *VCSBaseEditorWidget::codec() const
{
    return baseTextDocument()->codec();
}

void VCSBaseEditorWidget::setCodec(QTextCodec *c)
{
    if (c) {
        baseTextDocument()->setCodec(c);
    } else {
        qWarning("%s: Attempt to set 0 codec.", Q_FUNC_INFO);
    }
}

EditorContentType VCSBaseEditorWidget::contentType() const
{
    return d->m_parameters->type;
}

bool VCSBaseEditorWidget::isModified() const
{
    return false;
}

TextEditor::BaseTextEditor *VCSBaseEditorWidget::createEditor()
{
    TextEditor::BaseTextEditor *editor = 0;
    if (d->m_parameters->type == DiffOutput) {
        // Diff: set up diff file browsing
        VCSBaseDiffEditor *de = new VCSBaseDiffEditor(this, d->m_parameters);
        QComboBox *diffBrowseComboBox = de->diffFileBrowseComboBox();
        connect(diffBrowseComboBox, SIGNAL(activated(int)), this, SLOT(slotDiffBrowse(int)));
        editor = de;
    } else {
        editor = new VCSBaseEditor(this, d->m_parameters);
    }
    d->m_editor = editor;

    // Pass on signals.
    connect(this, SIGNAL(describeRequested(QString,QString)),
            editor, SIGNAL(describeRequested(QString,QString)));
    connect(this, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            editor, SIGNAL(annotateRevisionRequested(QString,QString,int)));
    return editor;
}

void VCSBaseEditorWidget::slotPopulateDiffBrowser()
{
    VCSBaseDiffEditor *de = static_cast<VCSBaseDiffEditor*>(editor());
    QComboBox *diffBrowseComboBox = de->diffFileBrowseComboBox();
    diffBrowseComboBox->clear();
    d->m_diffSections.clear();
    // Create a list of section line numbers (diffed files)
    // and populate combo with filenames.
    const QTextBlock cend = document()->end();
    int lineNumber = 0;
    QString lastFileName;
    for (QTextBlock it = document()->begin(); it != cend; it = it.next(), lineNumber++) {
        const QString text = it.text();
        // Check for a new diff section (not repeating the last filename)
        if (d->m_diffFilePattern.exactMatch(text)) {
            const QString file = fileNameFromDiffSpecification(it);
            if (!file.isEmpty() && lastFileName != file) {
                lastFileName = file;
                // ignore any headers
                d->m_diffSections.push_back(d->m_diffSections.empty() ? 0 : lineNumber);
                diffBrowseComboBox->addItem(QFileInfo(file).fileName());
            }
        }
    }
}

void VCSBaseEditorWidget::slotDiffBrowse(int index)
{
    // goto diffed file as indicated by index/line number
    if (index < 0 || index >= d->m_diffSections.size())
        return;
    const int lineNumber = d->m_diffSections.at(index) + 1; // TextEdit uses 1..n convention
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

void VCSBaseEditorWidget::slotDiffCursorPositionChanged()
{
    // Adapt diff file browse combo to new position
    // if the cursor goes across a file line.
    QTC_ASSERT(d->m_parameters->type == DiffOutput, return)
    const int newCursorLine = textCursor().blockNumber();
    if (newCursorLine == d->m_cursorLine)
        return;
    // Which section does it belong to?
    d->m_cursorLine = newCursorLine;
    const int section = sectionOfLine(d->m_cursorLine, d->m_diffSections);
    if (section != -1) {
        VCSBaseDiffEditor *de = static_cast<VCSBaseDiffEditor*>(editor());
        QComboBox *diffBrowseComboBox = de->diffFileBrowseComboBox();
        if (diffBrowseComboBox->currentIndex() != section) {
            const bool blocked = diffBrowseComboBox->blockSignals(true);
            diffBrowseComboBox->setCurrentIndex(section);
            diffBrowseComboBox->blockSignals(blocked);
        }
    }
}

QAction *VCSBaseEditorWidget::createDescribeAction(const QString &change)
{
    QAction *a = new QAction(tr("Describe change %1").arg(change), 0);
    connect(a, SIGNAL(triggered()), this, SLOT(describe()));
    return a;
}

QAction *VCSBaseEditorWidget::createAnnotateAction(const QString &change, bool previous)
{
    // Use 'previous' format if desired and available, else default to standard.
    const QString &format =  previous && !d->m_annotatePreviousRevisionTextFormat.isEmpty() ?
                d->m_annotatePreviousRevisionTextFormat : d->m_annotateRevisionTextFormat;
    QAction *a = new QAction(format.arg(change), 0);
    a->setData(change);
    connect(a, SIGNAL(triggered()), this, SLOT(slotAnnotateRevision()));
    return a;
}

QAction *VCSBaseEditorWidget::createCopyRevisionAction(const QString &change)
{
    QAction *a = new QAction(d->m_copyRevisionTextFormat.arg(change), 0);
    a->setData(change);
    connect(a, SIGNAL(triggered()), this, SLOT(slotCopyRevision()));
    return a;
}

void VCSBaseEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();
    // 'click on change-interaction'
    if (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput) {
        d->m_currentChange = changeUnderCursor(cursorForPosition(e->pos()));
        if (!d->m_currentChange.isEmpty()) {
            switch (d->m_parameters->type) {
            case LogOutput: // Describe current / Annotate file of current
                menu->addSeparator();
                menu->addAction(createCopyRevisionAction(d->m_currentChange));
                menu->addAction(createDescribeAction(d->m_currentChange));
                if (d->m_fileLogAnnotateEnabled)
                    menu->addAction(createAnnotateAction(d->m_currentChange, false));
                break;
            case AnnotateOutput: { // Describe current / annotate previous
                    menu->addSeparator();
                    menu->addAction(createCopyRevisionAction(d->m_currentChange));
                    menu->addAction(createDescribeAction(d->m_currentChange));
                    const QStringList previousVersions = annotationPreviousVersions(d->m_currentChange);
                    if (!previousVersions.isEmpty()) {
                        menu->addSeparator();
                        foreach(const QString &pv, previousVersions)
                            menu->addAction(createAnnotateAction(pv, true));
                    } // has previous versions
                }
                break;
            default:
                break;
            }         // switch type
        }             // has current change
    }
    menu->exec(e->globalPos());
    delete menu;
}

void VCSBaseEditorWidget::mouseMoveEvent(QMouseEvent *e)
{
    bool overrideCursor = false;
    Qt::CursorShape cursorShape;

    if (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput) {
        // Link emulation behaviour for 'click on change-interaction'
        QTextCursor cursor = cursorForPosition(e->pos());
        QString change = changeUnderCursor(cursor);
        if (!change.isEmpty()) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = cursor;
            sel.cursor.select(QTextCursor::WordUnderCursor);
            sel.format.setFontUnderline(true);
            sel.format.setProperty(QTextFormat::UserProperty, change);
            setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>() << sel);
            overrideCursor = true;
            cursorShape = Qt::PointingHandCursor;
        }
    } else {
        setExtraSelections(OtherSelection, QList<QTextEdit::ExtraSelection>());
        overrideCursor = true;
        cursorShape = Qt::IBeamCursor;
    }
    TextEditor::BaseTextEditorWidget::mouseMoveEvent(e);

    if (overrideCursor)
        viewport()->setCursor(cursorShape);
}

void VCSBaseEditorWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput) {
        if (e->button() == Qt::LeftButton &&!(e->modifiers() & Qt::ShiftModifier)) {
            QTextCursor cursor = cursorForPosition(e->pos());
            d->m_currentChange = changeUnderCursor(cursor);
            if (!d->m_currentChange.isEmpty()) {
                describe();
                e->accept();
                return;
            }
        }
    }
    TextEditor::BaseTextEditorWidget::mouseReleaseEvent(e);
}

void VCSBaseEditorWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (d->m_parameters->type == DiffOutput) {
        if (e->button() == Qt::LeftButton &&!(e->modifiers() & Qt::ShiftModifier)) {
            QTextCursor cursor = cursorForPosition(e->pos());
            jumpToChangeFromDiff(cursor);
        }
    }
    TextEditor::BaseTextEditorWidget::mouseDoubleClickEvent(e);
}

void VCSBaseEditorWidget::keyPressEvent(QKeyEvent *e)
{
    // Do not intercept return in editable patches.
    if (d->m_parameters->type == DiffOutput && isReadOnly()
        && (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)) {
        jumpToChangeFromDiff(textCursor());
        return;
    }
    BaseTextEditorWidget::keyPressEvent(e);
}

void VCSBaseEditorWidget::describe()
{
    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::describe" << d->m_currentChange;
    if (!d->m_currentChange.isEmpty())
        emit describeRequested(d->m_source, d->m_currentChange);
}

void VCSBaseEditorWidget::slotActivateAnnotation()
{
    // The annotation highlighting depends on contents (change number
    // set with assigned colors)
    if (d->m_parameters->type != AnnotateOutput)
        return;

    const QSet<QString> changes = annotationChanges();
    if (changes.isEmpty())
        return;
    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::slotActivateAnnotation(): #" << changes.size();

    disconnect(this, SIGNAL(textChanged()), this, SLOT(slotActivateAnnotation()));

    if (BaseAnnotationHighlighter *ah = qobject_cast<BaseAnnotationHighlighter *>(baseTextDocument()->syntaxHighlighter())) {
        ah->setChangeNumbers(changes);
        ah->rehighlight();
    } else {
        baseTextDocument()->setSyntaxHighlighter(createAnnotationHighlighter(changes));
    }
}

// Check for a change chunk "@@ -91,7 +95,7 @@" and return
// the modified line number (95).
// Note that git appends stuff after "  @@" (function names, etc.).
static inline bool checkChunkLine(const QString &line, int *modifiedLineNumber)
{
    if (!line.startsWith(QLatin1String("@@ ")))
        return false;
    const int endPos = line.indexOf(QLatin1String(" @@"), 3);
    if (endPos == -1)
        return false;
    // the first chunk range applies to the original file, the second one to
    // the modified file, the one we're interested int
    const int plusPos = line.indexOf(QLatin1Char('+'), 3);
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

void VCSBaseEditorWidget::jumpToChangeFromDiff(QTextCursor cursor)
{
    int chunkStart = 0;
    int lineCount = -1;
    const QChar deletionIndicator = QLatin1Char('-');
    // find nearest change hunk
    QTextBlock block = cursor.block();
    for ( ; block.isValid() ; block = block.previous()) {
        const QString line = block.text();
        if (checkChunkLine(line, &chunkStart)) {
            break;
        } else {
            if (!line.startsWith(deletionIndicator))
                ++lineCount;
        }
    }

    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::jumpToChangeFromDiff()1" << chunkStart << lineCount;

    if (chunkStart == -1 || lineCount < 0 || !block.isValid())
        return;

    // find the filename in previous line, map depot name back
    block = block.previous();
    if (!block.isValid())
        return;
    const QString fileName = fileNameFromDiffSpecification(block);

    const bool exists = fileName.isEmpty() ? false : QFile::exists(fileName);

    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::jumpToChangeFromDiff()2" << fileName << "ex=" << exists << "line" << chunkStart <<  lineCount;

    if (!exists)
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    Core::IEditor *ed = em->openEditor(fileName, QString(), Core::EditorManager::ModeSwitch);
    if (TextEditor::ITextEditor *editor = qobject_cast<TextEditor::ITextEditor *>(ed))
        editor->gotoLine(chunkStart + lineCount);
}

void VCSBaseEditorWidget::setPlainTextData(const QByteArray &data)
{
    if (data.size() > Core::EditorManager::maxTextFileSize()) {
        setPlainText(msgTextTooLarge(data.size()));
    } else {
        setPlainText(codec()->toUnicode(data));
    }
}

void VCSBaseEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    if (d->m_parameters->type == DiffOutput) {
        if (DiffHighlighter *highlighter = qobject_cast<DiffHighlighter*>(baseTextDocument()->syntaxHighlighter())) {
            static QVector<QString> categories;
            if (categories.isEmpty()) {
                categories << QLatin1String(TextEditor::Constants::C_TEXT)
                           << QLatin1String(TextEditor::Constants::C_ADDED_LINE)
                           << QLatin1String(TextEditor::Constants::C_REMOVED_LINE)
                           << QLatin1String(TextEditor::Constants::C_DIFF_FILE)
                           << QLatin1String(TextEditor::Constants::C_DIFF_LOCATION);
            }
            highlighter->setFormats(fs.toTextCharFormats(categories));
            highlighter->rehighlight();
        }
    }
}

const VCSBaseEditorParameters *VCSBaseEditorWidget::findType(const VCSBaseEditorParameters *array,
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
                if (VCSBase::Constants::Internal::debug)
                    qDebug() << Q_FUNC_INFO << source << codec->name();
                return codec;
            }
    }
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << source << "not found";
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
            if (const Core::IFile *file = (*it)->file())
                if (file->fileName().startsWith(dir)) {
                    QTextCodec *codec = (*it)->editorConfiguration()->textCodec();
                    if (VCSBase::Constants::Internal::debug)
                        qDebug() << Q_FUNC_INFO << dir << (*it)->displayName() << codec->name();
                    return codec;
                }
    }
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << dir << "not found";
    return 0;
}

QTextCodec *VCSBaseEditorWidget::getCodec(const QString &source)
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
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << source << "defaulting to " << sys->name();
    return sys;
}

QTextCodec *VCSBaseEditorWidget::getCodec(const QString &workingDirectory, const QStringList &files)
{
    if (files.empty())
        return getCodec(workingDirectory);
    return getCodec(workingDirectory + QLatin1Char('/') + files.front());
}

VCSBaseEditorWidget *VCSBaseEditorWidget::getVcsBaseEditor(const Core::IEditor *editor)
{
    if (const TextEditor::BaseTextEditor *be = qobject_cast<const TextEditor::BaseTextEditor *>(editor))
        return qobject_cast<VCSBaseEditorWidget *>(be->editorWidget());
    return 0;
}

// Return line number of current editor if it matches.
int VCSBaseEditorWidget::lineNumberOfCurrentEditor(const QString &currentFile)
{
    Core::IEditor *ed = Core::EditorManager::instance()->currentEditor();
    if (!ed)
        return -1;
    if (!currentFile.isEmpty()) {
        const Core::IFile *ifile  = ed->file();
        if (!ifile || ifile->fileName() != currentFile)
            return -1;
    }
    const TextEditor::BaseTextEditor *eda = qobject_cast<const TextEditor::BaseTextEditor *>(ed);
    if (!eda)
        return -1;
    return eda->currentLine();
}

bool VCSBaseEditorWidget::gotoLineOfEditor(Core::IEditor *e, int lineNumber)
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
QString VCSBaseEditorWidget::getSource(const QString &workingDirectory,
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

QString VCSBaseEditorWidget::getSource(const QString &workingDirectory,
                                 const QStringList &fileNames)
{
    return fileNames.size() == 1 ?
            getSource(workingDirectory, fileNames.front()) :
            workingDirectory;
}

QString VCSBaseEditorWidget::getTitleId(const QString &workingDirectory,
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

bool VCSBaseEditorWidget::setConfigurationWidget(QWidget *w)
{
    if (!d->m_editor || d->m_configurationWidget)
        return false;

    d->m_configurationWidget = w;
    d->m_editor->insertExtraToolBarWidget(TextEditor::BaseTextEditor::Right, w);

    return true;
}

QWidget *VCSBaseEditorWidget::configurationWidget() const
{
    return d->m_configurationWidget;
}

// Find the complete file from a diff relative specification.
QString VCSBaseEditorWidget::findDiffFile(const QString &f, Core::IVersionControl *control /* = 0 */) const
{
    // Try the file itself, expand to absolute.
    const QFileInfo in(f);
    if (in.isAbsolute())
        return in.isFile() ? f : QString();
    if (in.isFile())
        return in.absoluteFilePath();
    // 1) Try base dir
    const QChar slash = QLatin1Char('/');
    if (!d->m_diffBaseDirectory.isEmpty()) {
        const QFileInfo baseFileInfo(d->m_diffBaseDirectory + slash + f);
        if (baseFileInfo.isFile())
            return baseFileInfo.absoluteFilePath();
    }
    // 2) Try in source (which can be file or directory)
    if (source().isEmpty())
        return QString();
    const QFileInfo sourceInfo(source());
    const QString sourceDir = sourceInfo.isDir() ? sourceInfo.absoluteFilePath() : sourceInfo.absolutePath();
    const QFileInfo sourceFileInfo(sourceDir + slash + f);
    if (sourceFileInfo.isFile())
        return sourceFileInfo.absoluteFilePath();
    // Try to locate via repository.
    if (!control)
        return QString();
    QString topLevel;
    if (!control->managesDirectory(sourceDir, &topLevel))
        return QString();
    const QFileInfo topLevelFileInfo(topLevel + slash + f);
    if (topLevelFileInfo.isFile())
        return topLevelFileInfo.absoluteFilePath();
    return QString();
}

void VCSBaseEditorWidget::slotAnnotateRevision()
{
    if (const QAction *a = qobject_cast<const QAction *>(sender()))
        emit annotateRevisionRequested(source(), a->data().toString(),
                                       editor()->currentLine());
}

void VCSBaseEditorWidget::slotCopyRevision()
{
    QApplication::clipboard()->setText(d->m_currentChange);
}

QStringList VCSBaseEditorWidget::annotationPreviousVersions(const QString &) const
{
    return QStringList();
}

} // namespace VCSBase

#include "vcsbaseeditor.moc"
