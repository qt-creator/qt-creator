/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef VCSBASEPLUGIN_H
#define VCSBASEPLUGIN_H

#include "vcsbase_global.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
class QProcessEnvironment;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {
    struct SynchronousProcessResponse;
}

namespace Core {
    class IVersionControl;
}

namespace VCSBase {
namespace Internal {
    struct State;
}

class VCSBaseSubmitEditor;
struct VCSBasePluginPrivate;
class VCSBasePluginStateData;
class VCSBasePlugin;

/* VCSBasePlugin and VCSBasePluginState: Provide a base class for
 * VCS plugins. It mainly takes care of maintaining the
 * VCS-relevant state of Qt Creator which is a tuple of
 *
 * 1) Current file    and it's version system control/top level
 * 2) Current project and it's version system control/top level
 *
 * (reflected in VCSBasePluginState). The plugin connects to the
 * relevant change signals in Qt Creator and calls the virtual
 * updateActions() for the plugins to update their menu actions
 * according to the new state. This is done centrally to avoid
 * single plugins repeatedly invoking searches/QFileInfo on files,
 * etc.
 * Independently, there are accessors for current patch files, which return
 * a file name if the current file could be a patch file which could be applied
 * and a repository exists.
 *
 * If current file/project are managed
 * by different version controls, the project is discarded and only
 * the current file is taken into account, allowing to do a diff
 * also when the project of a file is not opened.
 *
 * When triggering an action, a copy of the state should be made to
 * keep it, as it may rapidly change due to context changes, etc.
 *
 * The class also detects the VCS plugin submit editor closing and calls
 * the virtual submitEditorAboutToClose() to trigger the submit process. */

class VCSBASE_EXPORT VCSBasePluginState
{
public:
    VCSBasePluginState();
    VCSBasePluginState(const VCSBasePluginState &);
    VCSBasePluginState &operator=(const VCSBasePluginState &);
    ~VCSBasePluginState();

    void clear();

    bool isEmpty() const;
    bool hasFile() const;
    bool hasPatchFile() const;
    bool hasProject() const;
    bool hasTopLevel() const;

    // Current file.
    QString currentFile() const;
    QString currentFileName() const;
    QString currentFileDirectory() const;
    QString currentFileTopLevel() const;
    // Convenience: Returns file relative to top level.
    QString relativeCurrentFile() const;

    // If the current file looks like a patch and there is a top level,
    // it will end up here (for VCS that offer patch functionality).
    QString currentPatchFile() const;
    QString currentPatchFileDisplayName() const;

    // Current project.
    QString currentProjectPath() const;
    QString currentProjectName() const;
    QString currentProjectTopLevel() const;
    /* Convenience: Returns project path relative to top level if it
     * differs from top level (else empty()) as an argument list to do
     * eg a 'vcs diff <args>' */
    QStringList relativeCurrentProject() const;

    // Top level directory for actions on the top level. Preferably
    // the file one.
    QString topLevel() const;

    bool equals(const VCSBasePluginState &rhs) const;

    friend VCSBASE_EXPORT QDebug operator<<(QDebug in, const VCSBasePluginState &state);

private:
    friend class VCSBasePlugin;
    bool equals(const Internal::State &s) const;
    void setState(const Internal::State &s);

    QSharedDataPointer<VCSBasePluginStateData> data;
};

VCSBASE_EXPORT QDebug operator<<(QDebug in, const VCSBasePluginState &state);

inline bool operator==(const VCSBasePluginState &s1, const VCSBasePluginState &s2)
{ return s1.equals(s2); }
inline bool operator!=(const VCSBasePluginState &s1, const VCSBasePluginState &s2)
{ return !s1.equals(s2); }

class VCSBASE_EXPORT VCSBasePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

protected:
    explicit VCSBasePlugin(const QString &submitEditorId);

    virtual void initialize(Core::IVersionControl *vc);
    virtual void extensionsInitialized();

public:
    virtual ~VCSBasePlugin();

    const VCSBasePluginState &currentState() const;
    Core::IVersionControl *versionControl() const;

    // For internal tests: Create actions driving IVersionControl's snapshot interface.
    QList<QAction*> createSnapShotTestActions();

    // Convenience that searches for the repository specifically for version control
    // systems that do not have directories like "CVS" in each managed subdirectory
    // but have a directory at the top of the repository like ".git" containing
    // a well known file. See implementation for gory details.
    static QString findRepositoryForDirectory(const QString &dir, const QString &checkFile);

    // Set up the environment for a version control command line call.
    // Sets up SSH graphical password prompting (note that the latter
    // requires a terminal-less process) and sets LANG to 'C' to force English
    // (suppress LOCALE warnings/parse commands output) if desired.
    static void setProcessEnvironment(QProcessEnvironment *e, bool forceCLocale);
    // Returns whether an SSH prompt is configured.
    static bool isSshPromptConfigured();

    // Convenience to synchronously run VCS commands
    enum RunVCSFlags {
        ShowStdOutInLogWindow = 0x1, // Append standard output to VCS output window.
        MergeOutputChannels = 0x2,   // see QProcess: Merge stderr/stdout.
        SshPasswordPrompt = 0x4,    // Disable terminal on UNIX to force graphical prompt.
        SuppressStdErrInLogWindow = 0x8, // No standard error output to VCS output window.
        SuppressFailMessageInLogWindow = 0x10, // No message VCS about failure in VCS output window.
        SuppressCommandLogging = 0x20, // No command log entry in VCS output window.
        ShowSuccessMessage = 0x40,      // Show message about successful completion in VCS output window.
        ForceCLocale = 0x80,            // Force C-locale for commands whose output is parsed.
        FullySynchronously = 0x100      // Suppress local event loop (in case UI actions are
                                        // triggered by file watchers).
    };

    static Utils::SynchronousProcessResponse
            runVCS(const QString &workingDir,
                   const QString &binary,
                   const QStringList &arguments,
                   int timeOutMS,
                   QProcessEnvironment env,
                   unsigned flags = 0,
                   QTextCodec *outputCodec = 0);

    static Utils::SynchronousProcessResponse
            runVCS(const QString &workingDir,
                   const QString &binary,
                   const QStringList &arguments,
                   int timeOutMS,
                   unsigned flags = 0,
                   QTextCodec *outputCodec = 0);

public slots:
    // Convenience slot for "Delete current file" action. Prompts to
    // delete the file via VCSManager.
    void promptToDeleteCurrentFile();
    // Prompt to initialize version control in a directory, initially
    // pointing to the current project.
    void createRepository();

protected:
    enum ActionState { NoVCSEnabled, OtherVCSEnabled, VCSEnabled };

    // Implement to enable the plugin menu actions according to state.
    virtual void updateActions(ActionState as) = 0;
    // Implement to start the submit process.
    virtual bool submitEditorAboutToClose(VCSBaseSubmitEditor *submitEditor) = 0;

    // A helper to enable the VCS menu action according to state:
    // NoVCSEnabled    -> visible, enabled if repository creation is supported
    // OtherVCSEnabled -> invisible
    // Else:           -> fully enabled.
    // Returns whether actions should be set up further.
    bool enableMenuAction(ActionState as, QAction *in) const;

private slots:
    void slotSubmitEditorAboutToClose(VCSBaseSubmitEditor *submitEditor, bool *result);
    void slotStateChanged(const VCSBase::Internal::State &s, Core::IVersionControl *vc);
    void slotTestSnapshot();
    void slotTestListSnapshots();
    void slotTestRestoreSnapshot();
    void slotTestRemoveSnapshot();

private:
    VCSBasePluginPrivate *d;
};

} // namespace VCSBase

#endif // VCSBASEPLUGIN_H
