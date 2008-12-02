/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef VCSBaseSUBMITEDITOR_H
#define VCSBaseSUBMITEDITOR_H

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QList>

namespace Core {
    namespace Utils {
        class SubmitEditorWidget;
    }
}

namespace VCSBase {

struct VCSBaseSubmitEditorPrivate;

/* Utility struct to parametrize a VCSBaseSubmitEditor. */
struct VCSBASE_EXPORT VCSBaseSubmitEditorParameters {
    const char *mimeType;
    const char *kind;
    const char *context;
    const char *undoActionId;
    const char *redoActionId;
    const char *submitActionId;
    const char *diffActionId;
};

/* Base class for a submit editor based on the Core::Utils::SubmitEditorWidget
 * that presents the commit message in a text editor and an
 * checkable list of modified files in a list window. The user can delete
 * files from the list by pressing unchecking them or diff the selection
 * by doubleclicking.
 *
 * The action matching the the ids (unless 0) of the parameter struct will be
 * registered with the EditorWidget and submit/diff actions will be added to
 * a toolbar.
 *
 * For the given context, there must be only one instance of the editor
 * active.
 * To start a submit, set the submit template on the editor and the output
 * of the VCS status command listing the modified files as fileList and open
 * it.
 * The submit process is started by listening on the editor close
 * signal and then asking the IFile interface of the editor to save the file
 * within a IFileManager::blockFileChange() section
 * and to launch the submit process. In addition, the action registered
 * for submit should be connected to a slot triggering the close of the
 * current editor in the editor manager. */

class VCSBASE_EXPORT VCSBaseSubmitEditor : public Core::IEditor
{
    Q_OBJECT
public:
    typedef QList<int> Context;

protected:
    explicit VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                 Core::Utils::SubmitEditorWidget *editorWidget);

public:
    virtual ~VCSBaseSubmitEditor();

    // Core::IEditor
    virtual bool createNew(const QString &contents);
    virtual bool open(const QString &fileName);
    virtual Core::IFile *file();
    virtual QString displayName() const;
    virtual void setDisplayName(const QString &title);
    virtual bool duplicateSupported() const;
    virtual Core::IEditor *duplicate(QWidget * parent);
    virtual const char *kind() const;

    virtual QToolBar *toolBar();
    virtual QList<int> context() const;
    virtual QWidget *widget();

    virtual QByteArray saveState() const;
    virtual bool restoreState(const QByteArray &state);

    QStringList checkedFiles() const;

    void setFileList(const QStringList&);
    void addFiles(const QStringList&, bool checked = true, bool userCheckable = true);

signals:
    void diffSelectedFiles(const QStringList &files);

private slots:
    void slotDiffSelectedVCSFiles(const QStringList &rawList);
    bool save(const QString &fileName);
    void slotDescriptionChanged();

protected:
    /* Implemented this to extract the real file list from the status
     * output of the versioning system as displayed in the file list
     * for example "M foo.cpp" -> "foo.cpp". */
    virtual QStringList vcsFileListToFileList(const QStringList &) const = 0;

    /* These hooks allow for modifying the contents that goes to
     * the file. The default implementation uses the text
     * of the description editor. */
    virtual QString fileContents() const;
    virtual bool setFileContents(const QString &contents);

private:
    VCSBaseSubmitEditorPrivate *m_d;
};

}

#endif // VCSBaseSUBMITEDITOR_H
