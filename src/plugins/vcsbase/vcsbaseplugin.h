// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/icontext.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/iplugin.h>

#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Utils { class Environment; }

namespace Core {
class Context;
class IVersionControl;
class IDocument;
} // namespace Core

namespace VcsBase {

namespace Internal { class State; }

class VcsBaseSubmitEditor;
class VcsBasePluginPrivate;
class VcsBasePluginStateData;
class VcsCommand;

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
    Utils::FilePath currentFile() const;
    QString currentFileName() const;
    Utils::FilePath currentFileDirectory() const;
    Utils::FilePath currentFileTopLevel() const;
    // Convenience: Returns file relative to top level.
    QString relativeCurrentFile() const;

    // If the current file looks like a patch and there is a top level,
    // it will end up here (for VCS that offer patch functionality).
    QString currentPatchFile() const;
    QString currentPatchFileDisplayName() const;

    // Current project.
    Utils::FilePath currentProjectPath() const;
    QString currentProjectName() const;
    Utils::FilePath currentProjectTopLevel() const;
    /* Convenience: Returns project path relative to top level if it
     * differs from top level (else empty string) as an argument list to do
     * eg a 'vcs diff <args>' */
    QString relativeCurrentProject() const;

    // Top level directory for actions on the top level. Preferably
    // the file one.
    Utils::FilePath topLevel() const;

    bool equals(const VcsBasePluginState &rhs) const;

    friend VCSBASE_EXPORT QDebug operator<<(QDebug in, const VcsBasePluginState &state);

    friend bool operator==(const VcsBasePluginState &s1, const VcsBasePluginState &s2)
    { return s1.equals(s2); }
    friend bool operator!=(const VcsBasePluginState &s1, const VcsBasePluginState &s2)
    { return !s1.equals(s2); }

private:
    friend class VcsBasePluginPrivate;
    bool equals(const Internal::State &s) const;
    void setState(const Internal::State &s);

    QSharedDataPointer<VcsBasePluginStateData> data;
};

// Convenience that searches for the repository specifically for version control
// systems that do not have directories like "CVS" in each managed subdirectory
// but have a directory at the top of the repository like ".git" containing
// a well known file. See implementation for gory details.
VCSBASE_EXPORT Utils::FilePath findRepositoryForFile(const Utils::FilePath &fileOrDir,
                                                     const QString &checkFile);

// Set up the environment for a version control command line call.
// Sets up SSH graphical password prompting (note that the latter
// requires a terminal-less process) and sets LANG to 'C' to force English
// (suppress LOCALE warnings/parse commands output) if desired.
VCSBASE_EXPORT void setProcessEnvironment(Utils::Environment *e);
// Sets the source of editor contents, can be directory or file.
VCSBASE_EXPORT void setSource(Core::IDocument *document, const Utils::FilePath &source);
// Returns the source of editor contents.
VCSBASE_EXPORT Utils::FilePath source(Core::IDocument *document);

class VCSBASE_EXPORT VcsBasePluginPrivate : public Core::IVersionControl
{
    Q_OBJECT

protected:
    explicit VcsBasePluginPrivate(const Core::Context &context);

public:
    void extensionsInitialized();

    const VcsBasePluginState &currentState() const;

    /*!
     * Return a VcsCommand capable of checking out \a url into \a baseDirectory, where
     * a new subdirectory with \a localName will be created.
     *
     * \a extraArgs are passed on to the command being run.
     */
    virtual VcsCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FilePath &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs);
    // Display name of the commit action
    virtual QString commitDisplayName() const;
    virtual QString commitAbortTitle() const;
    virtual QString commitAbortMessage() const;
    virtual QString commitErrorMessage(const QString &error) const;

    void commitFromEditor();
    virtual bool activateCommit() = 0;

protected:
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
    virtual void discardCommit();

    // A helper to enable the VCS menu action according to state:
    // NoVcsEnabled    -> visible, enabled if repository creation is supported
    // OtherVcsEnabled -> invisible
    // Else:           -> fully enabled.
    // Returns whether actions should be set up further.
    bool enableMenuAction(ActionState as, QAction *in) const;

private:
    void slotStateChanged(const Internal::State &s, Core::IVersionControl *vc);

    bool supportsRepositoryCreation() const;

    QPointer<VcsBaseSubmitEditor> m_submitEditor;
    Core::Context m_context;
    VcsBasePluginState m_state;
    int m_actionState = -1;
};

} // namespace VcsBase
