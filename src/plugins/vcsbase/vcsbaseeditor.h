// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/patchtool.h>
#include <texteditor/texteditor.h>

#include <QSet>
#include <QTextCodec>

QT_BEGIN_NAMESPACE
class QTextCursor;
QT_END_NAMESPACE

namespace VcsBase {

namespace Internal {
class ChangeTextCursorHandler;
class VcsBaseEditorWidgetPrivate;
} // namespace Internal

class BaseAnnotationHighlighter;
class VcsBaseEditorConfig;
class VcsBaseEditorWidget;
class VcsCommand;
class VcsEditorFactory;

// Documentation inside
enum EditorContentType
{
    LogOutput,
    AnnotateOutput,
    DiffOutput,
    OtherContent
};

class VCSBASE_EXPORT VcsBaseEditorParameters
{
public:
    EditorContentType type;
    const char *id;
    const char *displayName;
    const char *mimeType;
};

class VCSBASE_EXPORT DiffChunk
{
public:
    bool isValid() const;
    QByteArray asPatch(const Utils::FilePath &workingDirectory) const;

    Utils::FilePath fileName;
    QByteArray chunk;
    QByteArray header;
};

class VCSBASE_EXPORT VcsBaseEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    VcsBaseEditor();

    // Utility to find a parameter set by type in an array.
    static const VcsBaseEditorParameters *
    findType(const VcsBaseEditorParameters *array, int arraySize, EditorContentType et);

    // Utility to find the codec for a source (file or directory), querying
    // the editor manager and the project managers (defaults to system codec).
    // The codec should be set on editors displaying diff or annotation
    // output.
    static QTextCodec *getCodec(const Utils::FilePath &source);
    static QTextCodec *getCodec(const Utils::FilePath &workingDirectory, const QStringList &files);

    // Utility to return the widget from the IEditor returned by the editor
    // manager which is a BaseTextEditor.
    static VcsBaseEditorWidget *getVcsBaseEditor(const Core::IEditor *editor);

    // Utility to find the line number of the current editor. Optionally,
    // pass in the file name to match it. To be used when jumping to current
    // line number in a 'annnotate current file' slot, which checks if the
    // current file originates from the current editor or the project selection.
    static int lineNumberOfCurrentEditor(const Utils::FilePath &currentFile = {});

    //Helper to go to line of editor if it is a text editor
    static bool gotoLineOfEditor(Core::IEditor *e, int lineNumber);

    // Convenience functions to determine the source to pass on to a diff
    // editor if one has a call consisting of working directory and file arguments.
    // ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
    static Utils::FilePath getSource(const Utils::FilePath &workingDirectory, const QString &fileName);
    static Utils::FilePath getSource(const Utils::FilePath &workingDirectory, const QStringList &fileNames);
    // Convenience functions to determine an title/id to identify the editor
    // from the arguments (','-joined arguments or directory) + revision.
    static QString getTitleId(const Utils::FilePath &workingDirectory,
                              const QStringList &fileNames,
                              const QString &revision = {});

    /* Tagging editors: Sometimes, an editor should be re-used, for example, when showing
     * a diff of the same file with different diff-options. In order to be able to find
     * the editor, they get a 'tag' containing type and parameters (dynamic property string). */
    static void tagEditor(Core::IEditor *e, const QString &tag);
    static Core::IEditor* locateEditorByTag(const QString &tag);
    static QString editorTag(EditorContentType t, const Utils::FilePath &workingDirectory,
                             const QStringList &files, const QString &revision = {});
    void finalizeInitialization() override;
};

class VCSBASE_EXPORT VcsBaseEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

protected:
    // Initialization requires calling init() (which in turns calls
    // virtual functions).
    VcsBaseEditorWidget();
    // Pattern for diff header. File name must be in the first capture group
    void setDiffFilePattern(const QString &pattern);
    // Pattern for log entry. hash/revision number must be in the first capture group
    void setLogEntryPattern(const QString &pattern);
    // Pattern for annotation entry. hash/revision number must be in the first capture group
    void setAnnotationEntryPattern(const QString &pattern);
    // Pattern for annotation separator. Lookup will stop on match.
    void setAnnotationSeparatorPattern(const QString &pattern);
    virtual bool supportChangeLinks() const;
    virtual Utils::FilePath fileNameForLine(int line) const;

    QString lineNumber(int blockNumber) const override;
    int lineNumberDigits() const override;

public:
    typedef std::function<void(const Utils::FilePath &, const QString &)> DescribeFunc;

    void finalizeInitialization() override;
    // FIXME: Consolidate these into finalizeInitialization
    void setDescribeFunc(DescribeFunc describeFunc);
    // void
    virtual void init();
    //
    void setParameters(const VcsBaseEditorParameters *parameters);

    ~VcsBaseEditorWidget() override;

    /* Force read-only: Make it a read-only, temporary file.
     * Should be set to true by version control views. It is not on
     * by default since it should not  trigger when patches are opened as
     * files. */
    void setForceReadOnly(bool b);

    Utils::FilePath source() const;
    void setSource(const  Utils::FilePath &source);

    // Format for "Annotate" revision menu entries. Should contain '%1" placeholder
    QString annotateRevisionTextFormat() const;
    void setAnnotateRevisionTextFormat(const QString &);

    // Format for "Annotate Previous" revision menu entries. Should contain '%1" placeholder.
    // Defaults to "annotateRevisionTextFormat" if unset.
    QString annotatePreviousRevisionTextFormat() const;
    void setAnnotatePreviousRevisionTextFormat(const QString &);

    // Enable "Annotate" context menu in file log view
    // (set to true if the source is a single file and the VCS implements it)
    bool isFileLogAnnotateEnabled() const;
    void setFileLogAnnotateEnabled(bool e);

    void setHighlightingEnabled(bool e);

    QTextCodec *codec() const;
    void setCodec(QTextCodec *);

    // Base directory for diff views
    Utils::FilePath workingDirectory() const;
    void setWorkingDirectory(const Utils::FilePath &wd);

    int firstLineNumber() const;
    void setFirstLineNumber(int firstLineNumber);

    bool isModified() const;

    EditorContentType contentType() const;

    void setEditorConfig(VcsBaseEditorConfig *config);
    VcsBaseEditorConfig *editorConfig() const;

    void setCommand(VcsCommand *command);
    void setDefaultLineNumber(int line);
    void gotoDefaultLine();

    virtual void setPlainText(const QString &text);

signals:
    // These signals also exist in the opaque editable (IEditor) that is
    // handled by the editor manager for convenience. They are emitted
    // for LogOutput/AnnotateOutput content types.
    void describeRequested(const Utils::FilePath &source, const QString &change);
    void annotateRevisionRequested(const Utils::FilePath &workingDirectory, const QString &file,
                                   const QString &change, int lineNumber);
    void diffChunkReverted();

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *) override;

    /* A helper that can be used to locate a file in a diff in case it
     * is relative. Tries to derive the directory from base directory,
     * source and version control. */
    virtual QString findDiffFile(const QString &f) const;

    virtual void addDiffActions(QMenu *menu, const DiffChunk &chunk);

    virtual void addChangeActions(QMenu *menu, const QString &change);

    // Implement to return a set of change identifiers in
    // annotation mode
    QSet<QString> annotationChanges() const;
    // Implement to identify a change number at the cursor position
    virtual QString changeUnderCursor(const QTextCursor &) const = 0;
    // Factory functions for highlighters
    virtual BaseAnnotationHighlighter *createAnnotationHighlighter(const QSet<QString> &changes) const = 0;
    // Returns a local file name from the diff file specification
    // (text cursor at position above change hunk)
    QString fileNameFromDiffSpecification(const QTextBlock &inBlock, QString *header = nullptr) const;

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
    void slotActivateAnnotation();
    void slotPopulateDiffBrowser();
    void slotPopulateLogBrowser();
    void slotJumpToEntry(int);
    void slotCursorPositionChanged() override;
    void slotAnnotateRevision(const QString &change);
    void slotApplyDiffChunk(const DiffChunk &chunk, Core::PatchAction patchAction);
    void slotPaste();
    void showProgressIndicator();
    void hideProgressIndicator();

    bool canApplyDiffChunk(const DiffChunk &dc) const;
    // Revert a patch chunk. Default implementation uses patch.exe
    bool applyDiffChunk(const DiffChunk &dc, Core::PatchAction patchAction) const;

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
    static void testDiffFileResolving(const VcsEditorFactory &factory);
    static void testLogResolving(const VcsEditorFactory &factory,
                                 const QByteArray &data,
                                 const QByteArray &entry1,
                                 const QByteArray &entry2);
#endif
};

} // namespace VcsBase

Q_DECLARE_METATYPE(VcsBase::DiffChunk)
