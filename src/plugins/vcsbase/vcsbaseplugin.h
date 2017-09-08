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

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <extensionsystem/iplugin.h>

#include <QList>
#include <QProcessEnvironment>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {
class FileName;
class SynchronousProcessResponse;
} // namespace Utils

namespace Core {
class Context;
class IVersionControl;
class Id;
class IDocument;
} // namespace Core

namespace VcsBase {

namespace Internal { class State; }

class VcsBaseSubmitEditor;
class VcsBasePluginPrivate;
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
     * differs from top level (else empty string) as an argument list to do
     * eg a 'vcs diff <args>' */
    QString relativeCurrentProject() const;

    // Top level directory for actions on the top level. Preferably
    // the file one.
    QString topLevel() const;

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
    explicit VcsBasePlugin();

    template<class T, typename... Args>
    T *initializeVcs(const Core::Context &context, Args&&... args)
    {
        T *vc = Core::VcsManager::registerVersionControl<T>(std::forward<Args>(args)...);
        initializeVcs(vc, context);
        return vc;
    }

    void extensionsInitialized() override;

public:
    ~VcsBasePlugin() override;

    const VcsBasePluginState &currentState() const;
    Core::IVersionControl *versionControl() const;

    // Convenience that searches for the repository specifically for version control
    // systems that do not have directories like "CVS" in each managed subdirectory
    // but have a directory at the top of the repository like ".git" containing
    // a well known file. See implementation for gory details.
    static QString findRepositoryForDirectory(const QString &dir, const QString &checkFile);

    // Set up the environment for a version control command line call.
    // Sets up SSH graphical password prompting (note that the latter
    // requires a terminal-less process) and sets LANG to 'C' to force English
    // (suppress LOCALE warnings/parse commands output) if desired.
    static void setProcessEnvironment(QProcessEnvironment *e,
                                      bool forceCLocale,
                                      const QString &sshPasswordPrompt = sshPrompt());
    // Returns SSH prompt configured in settings.
    static QString sshPrompt();
    // Returns whether an SSH prompt is configured.
    static bool isSshPromptConfigured();

    // Sets the source of editor contents, can be directory or file.
    static void setSource(Core::IDocument *document, const QString &source);
    // Returns the source of editor contents.
    static QString source(Core::IDocument *document);

    static Utils::SynchronousProcessResponse runVcs(const QString &workingDir,
                                                    const Utils::FileName &binary,
                                                    const QStringList &arguments,
                                                    int timeOutS,
                                                    unsigned flags = 0,
                                                    QTextCodec *outputCodec = 0,
                                                    const QProcessEnvironment &env = QProcessEnvironment());

protected:
    // Display name of the commit action:
    virtual QString commitDisplayName() const;
    // Prompt to save all files before commit:
    bool promptBeforeCommit();

    // Convenience slot for "Delete current file" action. Prompts to
    // delete the file via VcsManager.
    void promptToDeleteCurrentFile();
    // Prompt to initialize version control in a directory, initially
    // pointing to the current project.
    void createRepository();

    enum ActionState { NoVcsEnabled, OtherVcsEnabled, VcsEnabled };

    // Sets the current submit editor for this specific version control plugin.
    // The plugin automatically checks if the submit editor is closed and calls
    // submitEditorAboutToClose().
    // The function raiseSubmitEditor can be used to check for a running submit editor and raise it.
    void setSubmitEditor(VcsBaseSubmitEditor *submitEditor);
    // Current submit editor set through setSubmitEditor, if it wasn't closed inbetween
    VcsBaseSubmitEditor *submitEditor() const;
    // Tries to raise the submit editor set through setSubmitEditor. Returns true if that was found.
    bool raiseSubmitEditor() const;

    // Implement to enable the plugin menu actions according to state.
    virtual void updateActions(ActionState as) = 0;
    // Implement to start the submit process, use submitEditor() to get the submit editor instance.
    virtual bool submitEditorAboutToClose() = 0;

    // A helper to enable the VCS menu action according to state:
    // NoVcsEnabled    -> visible, enabled if repository creation is supported
    // OtherVcsEnabled -> invisible
    // Else:           -> fully enabled.
    // Returns whether actions should be set up further.
    bool enableMenuAction(ActionState as, QAction *in) const;

private:
    void slotSubmitEditorAboutToClose(VcsBaseSubmitEditor *submitEditor, bool *result);
    void slotStateChanged(const VcsBase::Internal::State &s, Core::IVersionControl *vc);

    void initializeVcs(Core::IVersionControl *vc, const Core::Context &context);

    VcsBasePluginPrivate *d;
};

} // namespace VcsBase
