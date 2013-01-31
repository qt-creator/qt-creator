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

#ifndef VCSBASEPLUGIN_H
#define VCSBASEPLUGIN_H

#include "vcsbase_global.h"

#include <extensionsystem/iplugin.h>

#include <QSharedDataPointer>
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
class QProcessEnvironment;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils { struct SynchronousProcessResponse; }

namespace Core { class IVersionControl; }

namespace VcsBase {

namespace Internal { struct State; }

class VcsBaseSubmitEditor;
struct VcsBasePluginPrivate;
class VcsBasePluginStateData;
class VcsBasePlugin;

// Documentation inside.
class VCSBASE_EXPORT VcsBasePluginState
{
public:
    VcsBasePluginState();
    VcsBasePluginState(const VcsBasePluginState &);
    VcsBasePluginState &operator=(const VcsBasePluginState &);
    ~VcsBasePluginState();

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

    QString currentDirectoryOrTopLevel() const;
    bool equals(const VcsBasePluginState &rhs) const;

    friend VCSBASE_EXPORT QDebug operator<<(QDebug in, const VcsBasePluginState &state);

private:
    friend class VcsBasePlugin;
    bool equals(const Internal::State &s) const;
    void setState(const Internal::State &s);

    QSharedDataPointer<VcsBasePluginStateData> data;
};

VCSBASE_EXPORT QDebug operator<<(QDebug in, const VcsBasePluginState &state);

inline bool operator==(const VcsBasePluginState &s1, const VcsBasePluginState &s2)
{ return s1.equals(s2); }
inline bool operator!=(const VcsBasePluginState &s1, const VcsBasePluginState &s2)
{ return !s1.equals(s2); }

class VCSBASE_EXPORT VcsBasePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

protected:
    explicit VcsBasePlugin(const QString &submitEditorId);

    void initializeVcs(Core::IVersionControl *vc);
    virtual void extensionsInitialized();

public:
    virtual ~VcsBasePlugin();

    const VcsBasePluginState &currentState() const;
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
    enum RunVcsFlags {
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

    static Utils::SynchronousProcessResponse runVcs(const QString &workingDir,
                                                    const QString &binary,
                                                    const QStringList &arguments,
                                                    int timeOutMS,
                                                    QProcessEnvironment env,
                                                    unsigned flags = 0,
                                                    QTextCodec *outputCodec = 0);

    static Utils::SynchronousProcessResponse runVcs(const QString &workingDir,
                                                    const QString &binary,
                                                    const QStringList &arguments,
                                                    int timeOutMS,
                                                    unsigned flags = 0,
                                                    QTextCodec *outputCodec = 0);

    // Make sure to not pass through the event loop at all:
    static bool runFullySynchronous(const QString &workingDirectory,
                                    const QString &binary,
                                    const QStringList &arguments,
                                    const QProcessEnvironment &env,
                                    QByteArray* outputText,
                                    QByteArray *errorText, int timeoutMS, bool logCommandToWindow);

    // Utility to run the 'patch' command
    static bool runPatch(const QByteArray &input, const QString &workingDirectory = QString(),
                         int strip = 0, bool reverse = false);

public slots:
    // Convenience slot for "Delete current file" action. Prompts to
    // delete the file via VcsManager.
    void promptToDeleteCurrentFile();
    // Prompt to initialize version control in a directory, initially
    // pointing to the current project.
    void createRepository();

protected:
    enum ActionState { NoVcsEnabled, OtherVcsEnabled, VcsEnabled };

    // Implement to enable the plugin menu actions according to state.
    virtual void updateActions(ActionState as) = 0;
    // Implement to start the submit process.
    virtual bool submitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor) = 0;

    // A helper to enable the VCS menu action according to state:
    // NoVcsEnabled    -> visible, enabled if repository creation is supported
    // OtherVcsEnabled -> invisible
    // Else:           -> fully enabled.
    // Returns whether actions should be set up further.
    bool enableMenuAction(ActionState as, QAction *in) const;

private slots:
    void slotSubmitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor, bool *result);
    void slotStateChanged(const VcsBase::Internal::State &s, Core::IVersionControl *vc);
    void slotTestSnapshot();
    void slotTestListSnapshots();
    void slotTestRestoreSnapshot();
    void slotTestRemoveSnapshot();

private:
    VcsBasePluginPrivate *d;
};

} // namespace VcsBase

#endif // VCSBASEPLUGIN_H
