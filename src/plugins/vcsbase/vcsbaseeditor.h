/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef VCSBASE_BASEEDITOR_H
#define VCSBASE_BASEEDITOR_H

#include "vcsbase_global.h"

#include <texteditor/basetexteditor.h>

#include <QtCore/QSet>

QT_BEGIN_NAMESPACE
class QAction;
class QTextCodec;
class QTextCursor;
QT_END_NAMESPACE

namespace Core {
    class IVersionControl;
}

namespace VCSBase {

struct VCSBaseEditorPrivate;
class DiffHighlighter;
class BaseAnnotationHighlighter;

// Contents of a VCSBaseEditor and its interaction.
enum EditorContentType {
    // No special handling.
    RegularCommandOutput,
    // Log of a file under revision control. Provide  'click on change'
    // description and 'Annotate' if is the log of a single file.
    LogOutput,
    // <change description>: file line
    // Color per change number and provide 'click on change' description.
    // Context menu offers "Annotate previous version".
    AnnotateOutput,
    // Diff output. Might includes describe output, which consists of a
    // header and diffs. Interaction is 'double click in  hunk' which
    // opens the file
    DiffOutput
};

// Helper struct used to parametrize an editor with mime type, context
// and id. The extension is currently only a suggestion when running
// VCS commands with redirection.
struct VCSBASE_EXPORT VCSBaseEditorParameters {
    EditorContentType type;
    const char *id;
    const char *displayName;
    const char *context;
    const char *mimeType;
    const char *extension;
};

// Base class for editors showing version control system output
// of the type enumerated by EditorContentType.
// The source property should contain the file or directory the log
// refers to and will be emitted with describeRequested().
// This is for VCS that need a current directory.
class VCSBASE_EXPORT VCSBaseEditor : public TextEditor::BaseTextEditor
{
    Q_PROPERTY(QString source READ source WRITE setSource)
    Q_PROPERTY(QString diffBaseDirectory READ diffBaseDirectory WRITE setDiffBaseDirectory)
    Q_PROPERTY(QTextCodec *codec READ codec WRITE setCodec)
    Q_PROPERTY(QString annotateRevisionTextFormat READ annotateRevisionTextFormat WRITE setAnnotateRevisionTextFormat)
    Q_PROPERTY(QString copyRevisionTextFormat READ copyRevisionTextFormat WRITE setCopyRevisionTextFormat)
    Q_PROPERTY(bool isFileLogAnnotateEnabled READ isFileLogAnnotateEnabled WRITE setFileLogAnnotateEnabled)
    Q_OBJECT
protected:
    // Initialization requires calling init() (which in turns calls
    // virtual functions).
    explicit VCSBaseEditor(const VCSBaseEditorParameters *type,
                           QWidget *parent);
public:
    void init();

    virtual ~VCSBaseEditor();

    QString source() const;
    void setSource(const  QString &source);

    // Format for "Annotate" revision menu entries. Should contain '%1" placeholder
    QString annotateRevisionTextFormat() const;
    void setAnnotateRevisionTextFormat(const QString &);

    // Format for "Copy" revision menu entries. Should contain '%1" placeholder
    QString copyRevisionTextFormat() const;
    void setCopyRevisionTextFormat(const QString &);

    // Enable "Annotate" context menu in file log view
    // (set to true if the source is a single file and the VCS implements it)
    bool isFileLogAnnotateEnabled() const;
    void setFileLogAnnotateEnabled(bool e);

    QTextCodec *codec() const;
    void setCodec(QTextCodec *);

    // Base directory for diff views
    QString diffBaseDirectory() const;
    void setDiffBaseDirectory(const QString &d);

    bool isModified() const;

    EditorContentType contentType() const;

    // Utility to find a parameter set by type in an array.
    static  const VCSBaseEditorParameters *
        findType(const VCSBaseEditorParameters *array, int arraySize, EditorContentType et);

    // Utility to find the codec for a source (file or directory), querying
    // the editor manager and the project managers (defaults to system codec).
    // The codec should be set on editors displaying diff or annotation
    // output.
    static QTextCodec *getCodec(const QString &source);
    static QTextCodec *getCodec(const QString &workingDirectory, const QStringList &files);

    // Utility to return the editor from the IEditor returned by the editor
    // manager which is a BaseTextEditable.
    static VCSBaseEditor *getVcsBaseEditor(const Core::IEditor *editor);

    // Utility to find the line number of the current editor. Optionally,
    // pass in the file name to match it. To be used when jumping to current
    // line number in a 'annnotate current file' slot, which checks if the
    // current file originates from the current editor or the project selection.
    static int lineNumberOfCurrentEditor(const QString &currentFile = QString());

    //Helper to go to line of editor if it is a text editor
    static bool gotoLineOfEditor(Core::IEditor *e, int lineNumber);

    // Convenience functions to determine the source to pass on to a diff
    // editor if one has a call consisting of working directory and file arguments.
    // ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
    static QString getSource(const QString &workingDirectory, const QString &fileName);
    static QString getSource(const QString &workingDirectory, const QStringList &fileNames);
    // Convenience functions to determine an title/id to identify the editor
    // from the arguments (','-joined arguments or directory) + revision.
    static QString getTitleId(const QString &workingDirectory,
                              const QStringList &fileNames,
                              const QString &revision = QString());
signals:
    // These signals also exist in the opaque editable (IEditor) that is
    // handled by the editor manager for convenience. They are emitted
    // for LogOutput/AnnotateOutput content types.
    void describeRequested(const QString &source, const QString &change);
    void annotateRevisionRequested(const QString &source, const QString &change, int lineNumber);

public slots:
    // Convenience slot to set data read from stdout, will use the
    // documents' codec to decode
    void setPlainTextData(const QByteArray &data);

protected:
    virtual TextEditor::BaseTextEditorEditable *createEditableInterface();

    void contextMenuEvent(QContextMenuEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);

public slots:
    void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void describe();
    void slotActivateAnnotation();
    void slotPopulateDiffBrowser();
    void slotDiffBrowse(int);
    void slotDiffCursorPositionChanged();
    void slotAnnotateRevision();
    void slotCopyRevision();

protected:
    /* A helper that can be used to locate a file in a diff in case it
     * is relative. Tries to derive the directory from base directory,
     * source and version control. */
    QString findDiffFile(const QString &f, Core::IVersionControl *control = 0) const;

private:
    // Implement to return a set of change identifiers in
    // annotation mode
    virtual QSet<QString> annotationChanges() const = 0;
    // Implement to identify a change number at the cursor position
    virtual QString changeUnderCursor(const QTextCursor &) const = 0;
    // Factory functions for highlighters
    virtual DiffHighlighter *createDiffHighlighter() const = 0;
    virtual BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const = 0;
    // Implement to return a local file name from the diff file specification
    // (text cursor at position above change hunk)
    virtual QString fileNameFromDiffSpecification(const QTextBlock &diffFileSpec) const = 0;
    // Implement to return the previous version[s] of an annotation change
    // for "Annotate previous version"
    virtual QStringList annotationPreviousVersions(const QString &revision) const;

    void jumpToChangeFromDiff(QTextCursor cursor);
    QAction *createDescribeAction(const QString &change);
    QAction *createAnnotateAction(const QString &change);
    QAction *createCopyRevisionAction(const QString &change);

    VCSBaseEditorPrivate *d;
};

} // namespace VCSBase

#endif // VCSBASE_BASEEDITOR_H
