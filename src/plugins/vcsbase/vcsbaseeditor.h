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

#ifndef VCSBASE_BASEEDITOR_H
#define VCSBASE_BASEEDITOR_H

#include "vcsbase_global.h"

#include <texteditor/basetexteditor.h>

#include <QSet>

QT_BEGIN_NAMESPACE
class QAction;
class QRegExp;
class QTextCodec;
class QTextCursor;
QT_END_NAMESPACE

namespace Core { class IVersionControl; }

namespace VcsBase {

namespace Internal {
class ChangeTextCursorHandler;
class VcsBaseEditorWidgetPrivate;
}

class DiffHighlighter;
class BaseAnnotationHighlighter;

// Documentation inside
enum EditorContentType
{
    RegularCommandOutput,
    LogOutput,
    AnnotateOutput,
    DiffOutput
};

class VCSBASE_EXPORT VcsBaseEditorParameters
{
public:
    EditorContentType type;
    const char *id;
    const char *displayName;
    const char *context;
    const char *mimeType;
    const char *extension;
};

class VCSBASE_EXPORT DiffChunk
{
public:
    bool isValid() const;
    QByteArray asPatch() const;

    QString fileName;
    QByteArray chunk;
};

class VCSBASE_EXPORT VcsBaseEditorWidget : public TextEditor::BaseTextEditorWidget
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
    explicit VcsBaseEditorWidget(const VcsBaseEditorParameters *type,
                                 QWidget *parent);
    // Pattern for diff header. File name must be in the first capture group
    void setDiffFilePattern(const QRegExp &pattern);
    // Pattern for log entry. hash/revision number must be in the first capture group
    void setLogEntryPattern(const QRegExp &pattern);

public:
    void init();

    ~VcsBaseEditorWidget();

    /* Force read-only: Make it a read-only, temporary file.
     * Should be set to true by version control views. It is not on
     * by default since it should not  trigger when patches are opened as
     * files. */
    void setForceReadOnly(bool b);

    QString source() const;
    void setSource(const  QString &source);

    // Format for "Annotate" revision menu entries. Should contain '%1" placeholder
    QString annotateRevisionTextFormat() const;
    void setAnnotateRevisionTextFormat(const QString &);

    // Format for "Annotate Previous" revision menu entries. Should contain '%1" placeholder.
    // Defaults to "annotateRevisionTextFormat" if unset.
    QString annotatePreviousRevisionTextFormat() const;
    void setAnnotatePreviousRevisionTextFormat(const QString &);

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
    static  const VcsBaseEditorParameters *
        findType(const VcsBaseEditorParameters *array, int arraySize, EditorContentType et);

    // Utility to find the codec for a source (file or directory), querying
    // the editor manager and the project managers (defaults to system codec).
    // The codec should be set on editors displaying diff or annotation
    // output.
    static QTextCodec *getCodec(const QString &source);
    static QTextCodec *getCodec(const QString &workingDirectory, const QStringList &files);

    // Utility to return the widget from the IEditor returned by the editor
    // manager which is a BaseTextEditor.
    static VcsBaseEditorWidget *getVcsBaseEditor(const Core::IEditor *editor);

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

    bool setConfigurationWidget(QWidget *w);
    QWidget *configurationWidget() const;

    /* Tagging editors: Sometimes, an editor should be re-used, for example, when showing
     * a diff of the same file with different diff-options. In order to be able to find
     * the editor, they get a 'tag' containing type and parameters (dynamic property string). */
    static void tagEditor(Core::IEditor *e, const QString &tag);
    static Core::IEditor* locateEditorByTag(const QString &tag);
    static QString editorTag(EditorContentType t, const QString &workingDirectory, const QStringList &files,
                             const QString &revision = QString());
signals:
    // These signals also exist in the opaque editable (IEditor) that is
    // handled by the editor manager for convenience. They are emitted
    // for LogOutput/AnnotateOutput content types.
    void describeRequested(const QString &source, const QString &change);
    void annotateRevisionRequested(const QString &source, const QString &change, int lineNumber);
    void diffChunkApplied(const VcsBase::DiffChunk &dc);
    void diffChunkReverted(const VcsBase::DiffChunk &dc);

public slots:
    // Convenience slot to set data read from stdout, will use the
    // documents' codec to decode
    void setPlainTextData(const QByteArray &data);

protected:
    virtual TextEditor::BaseTextEditor *createEditor();

    void contextMenuEvent(QContextMenuEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *);

public slots:
    void setFontSettings(const TextEditor::FontSettings &);

private slots:
    void slotActivateAnnotation();
    void slotPopulateDiffBrowser();
    void slotPopulateLogBrowser();
    void slotJumpToEntry(int);
    void slotCursorPositionChanged();
    void slotAnnotateRevision();
    void slotApplyDiffChunk();
    void slotPaste();

protected:
    /* A helper that can be used to locate a file in a diff in case it
     * is relative. Tries to derive the directory from base directory,
     * source and version control. */
    virtual QString findDiffFile(const QString &f) const;

    virtual void addChangeActions(QMenu *menu, const QString &change);

    // Implement to return a set of change identifiers in
    // annotation mode
    virtual QSet<QString> annotationChanges() const = 0;
    // Implement to identify a change number at the cursor position
    virtual QString changeUnderCursor(const QTextCursor &) const = 0;
    // Factory functions for highlighters
    virtual BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes,
                                                                   const QColor &bg) const = 0;
    // Returns a local file name from the diff file specification
    // (text cursor at position above change hunk)
    QString fileNameFromDiffSpecification(const QTextBlock &inBlock) const;

    // Implement to return decorated annotation change for "Annotate version"
    virtual QString decorateVersion(const QString &revision) const;
    // Implement to return the previous version[s] of an annotation change
    // for "Annotate previous version"
    virtual QStringList annotationPreviousVersions(const QString &revision) const;
    // Implement to validate revisions
    virtual bool isValidRevision(const QString &revision) const;
    // Implement to return subject for a change line in log
    virtual QString revisionSubject(const QTextBlock &inBlock) const;

private:
    bool canApplyDiffChunk(const DiffChunk &dc) const;
    // Revert a patch chunk. Default implementation uses patch.exe
    bool applyDiffChunk(const DiffChunk &dc, bool revert = false) const;

    // Indicates if the editor has diff contents. If true, an appropriate
    // highlighter is used and double-click inside a diff chunk jumps to
    // the relevant file and line
    bool hasDiff() const;

    // cut out chunk and determine file name.
    DiffChunk diffChunk(QTextCursor cursor) const;

    void jumpToChangeFromDiff(QTextCursor cursor);

    friend class Internal::ChangeTextCursorHandler;
    Internal::VcsBaseEditorWidgetPrivate *const d;

#ifdef WITH_TESTS
public:
    void testDiffFileResolving();
    void testLogResolving(QByteArray &data, const QByteArray &entry1, const QByteArray &entry2);
#endif
};

} // namespace VcsBase

Q_DECLARE_METATYPE(VcsBase::DiffChunk)

#endif // VCSBASE_BASEEDITOR_H
