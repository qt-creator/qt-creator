/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "vcsbaseeditor.h"
#include "diffhighlighter.h"
#include "baseannotationhighlighter.h"
#include "vcsbasetextdocument.h"
#include "vcsbaseconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QLayout>
#include <QtGui/QMenu>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>

namespace VCSBase {

// VCSBaseEditorEditable: An editable with no support for duplicates
class VCSBaseEditorEditable : public TextEditor::BaseTextEditorEditable
{
public:
    VCSBaseEditorEditable(VCSBaseEditor *,
                          const VCSBaseEditorParameters *type);
    QList<int> context() const;

    bool duplicateSupported() const { return false; }
    Core::IEditor *duplicate(QWidget * /*parent*/) { return 0; }
    const char *kind() const { return m_kind; }

private:
    const char *m_kind;
    QList<int> m_context;

};

VCSBaseEditorEditable::VCSBaseEditorEditable(VCSBaseEditor *editor,
                                             const VCSBaseEditorParameters *type)
    : BaseTextEditorEditable(editor), m_kind(type->kind)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(QLatin1String(type->context))
              << uidm->uniqueIdentifier(QLatin1String(TextEditor::Constants::C_TEXTEDITOR));
}

QList<int> VCSBaseEditorEditable::context() const
{
    return m_context;
}

// ----------- VCSBaseEditorPrivate

struct VCSBaseEditorPrivate
{
    VCSBaseEditorPrivate(const VCSBaseEditorParameters *type, QObject *parent);

    const VCSBaseEditorParameters *m_parameters;
    QAction *m_describeAction;
    QString m_currentChange;
    QString m_source;
};

VCSBaseEditorPrivate::VCSBaseEditorPrivate(const VCSBaseEditorParameters *type, QObject *parent)
    : m_parameters(type), m_describeAction(new QAction(parent))
{
}

// ------------ VCSBaseEditor
VCSBaseEditor::VCSBaseEditor(const VCSBaseEditorParameters *type, QWidget *parent)
  : BaseTextEditor(parent),
    d(new VCSBaseEditorPrivate(type, this))
{
    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::VCSBaseEditor" << type->type << type->kind;

    setReadOnly(true);
    connect(d->m_describeAction, SIGNAL(triggered()), this, SLOT(describe()));
    viewport()->setMouseTracking(true);
    setBaseTextDocument(new Internal::VCSBaseTextDocument);
    setMimeType(QLatin1String(d->m_parameters->mimeType));
}

void VCSBaseEditor::init()
{
    switch (d->m_parameters->type) {
    case RegularCommandOutput:
    case LogOutput:
    case AnnotateOutput:
        // Annotation highlighting depends on contents, which is set later on
        connect(this, SIGNAL(textChanged()), this, SLOT(slotActivateAnnotation()));
        break;
    case DiffOutput:
        baseTextDocument()->setSyntaxHighlighter(createDiffHighlighter());
        break;
    }
}

VCSBaseEditor::~VCSBaseEditor()
{
    delete d;
}

QString VCSBaseEditor::source() const
{
    return d->m_source;
}

void VCSBaseEditor::setSource(const  QString &source)
{
    d->m_source = source;
}

QTextCodec *VCSBaseEditor::codec() const
{
    return baseTextDocument()->codec();
}

void VCSBaseEditor::setCodec(QTextCodec *c)
{
    if (c) {
        baseTextDocument()->setCodec(c);
    } else {
        qWarning("%s: Attempt to set 0 codec.", Q_FUNC_INFO);
    }
}

EditorContentType VCSBaseEditor::contentType() const
{
    return d->m_parameters->type;
}

bool VCSBaseEditor::isModified() const
{
    return false;
}

TextEditor::BaseTextEditorEditable *VCSBaseEditor::createEditableInterface()
{
    return new VCSBaseEditorEditable(this, d->m_parameters);
}

void VCSBaseEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();
    // 'click on change-interaction'
    if (d->m_parameters->type == LogOutput || d->m_parameters->type == AnnotateOutput) {
        d->m_currentChange = changeUnderCursor(cursorForPosition(e->pos()));
        if (!d->m_currentChange.isEmpty()) {
            d->m_describeAction->setText(tr("Describe change %1").arg(d->m_currentChange));
            menu->addSeparator();
            menu->addAction(d->m_describeAction);
        }
    }
    menu->exec(e->globalPos());
    delete menu;
}

void VCSBaseEditor::mouseMoveEvent(QMouseEvent *e)
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
            change = changeUnderCursor(cursor);
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
    TextEditor::BaseTextEditor::mouseMoveEvent(e);

    if (overrideCursor)
        viewport()->setCursor(cursorShape);
}

void VCSBaseEditor::mouseReleaseEvent(QMouseEvent *e)
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
    TextEditor::BaseTextEditor::mouseReleaseEvent(e);
}

void VCSBaseEditor::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (d->m_parameters->type == DiffOutput) {
        if (e->button() == Qt::LeftButton &&!(e->modifiers() & Qt::ShiftModifier)) {
            QTextCursor cursor = cursorForPosition(e->pos());
            jumpToChangeFromDiff(cursor);
        }
    }
    TextEditor::BaseTextEditor::mouseDoubleClickEvent(e);
}

void VCSBaseEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        jumpToChangeFromDiff(textCursor());
        return;
    }
    BaseTextEditor::keyPressEvent(e);
}

void VCSBaseEditor::describe()
{
    if (VCSBase::Constants::Internal::debug)
        qDebug() << "VCSBaseEditor::describe" << d->m_currentChange;
    if (!d->m_currentChange.isEmpty())
        emit describeRequested(d->m_source, d->m_currentChange);
}

void VCSBaseEditor::slotActivateAnnotation()
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

void VCSBaseEditor::jumpToChangeFromDiff(QTextCursor cursor)
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
    Core::IEditor *ed = em->openEditor(fileName);
    em->ensureEditorManagerVisible();
    if (TextEditor::ITextEditor *editor = qobject_cast<TextEditor::ITextEditor *>(ed))
        editor->gotoLine(chunkStart + lineCount);
}

void VCSBaseEditor::setPlainTextData(const QByteArray &data)
{
    setPlainText(codec()->toUnicode(data));
}

void VCSBaseEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
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

const VCSBaseEditorParameters *VCSBaseEditor::findType(const VCSBaseEditorParameters *array,
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
            if (const TextEditor::BaseTextEditorEditable *be = qobject_cast<const TextEditor::BaseTextEditorEditable *>(*it)) {
                QTextCodec *codec = be->editor()->textCodec();
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
                    QTextCodec *codec = (*it)->editorConfiguration()->defaultTextCodec();
                    if (VCSBase::Constants::Internal::debug)
                        qDebug() << Q_FUNC_INFO << dir << (*it)->name() << codec->name();
                    return codec;
                }
    }
    if (VCSBase::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << dir << "not found";
    return 0;
}

QTextCodec *VCSBaseEditor::getCodec(const QString &source)
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

VCSBaseEditor *VCSBaseEditor::getVcsBaseEditor(const Core::IEditor *editor)
{
    if (const TextEditor::BaseTextEditorEditable *be = qobject_cast<const TextEditor::BaseTextEditorEditable *>(editor))
        return qobject_cast<VCSBaseEditor *>(be->editor());
    return 0;
}

} // namespace VCSBase
