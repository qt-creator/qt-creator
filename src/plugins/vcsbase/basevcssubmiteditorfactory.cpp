// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basevcssubmiteditorfactory.h"

#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <utils/qtcassert.h>

using namespace Core;

namespace VcsBase {

const char SUBMIT[] = "Vcs.Submit";
const char DIFF_SELECTED[] = "Vcs.DiffSelectedFiles";

class VcsSubmitEditorFactory final : public IEditorFactory
{
public:
    VcsSubmitEditorFactory(VersionControlBase *versionControl,
                           const VcsBaseSubmitEditorParameters &parameters)
    {
        QAction *submitAction = nullptr;
        QAction *diffAction = nullptr;
        QAction *undoAction = nullptr;
        QAction *redoAction = nullptr;

        const Context context(parameters.id);

        ActionBuilder(versionControl, Core::Constants::UNDO)
            .setText(Tr::tr("&Undo"))
            .setContext(context)
            .bindContextAction(&undoAction);

        ActionBuilder(versionControl, Core::Constants::REDO)
            .setText(Tr::tr("&Redo"))
            .setContext(context)
            .bindContextAction(&redoAction);

        ActionBuilder(versionControl, SUBMIT)
            .setText(versionControl->commitDisplayName())
            .setIcon(VcsBaseSubmitEditor::submitIcon())
            .setContext(context)
            .bindContextAction(&submitAction)
            .setCommandAttribute(Command::CA_UpdateText)
            .addOnTriggered(versionControl, &VersionControlBase::commitFromEditor);

        ActionBuilder(versionControl, DIFF_SELECTED)
            .setText(Tr::tr("Diff &Selected Files"))
            .setIcon(VcsBaseSubmitEditor::diffIcon())
            .setContext(context)
            .bindContextAction(&diffAction);

        setId(parameters.id);
        setDisplayName(QLatin1String(parameters.displayName));
        addMimeType(QLatin1String(parameters.mimeType));
        setEditorCreator([parameters, submitAction, diffAction, undoAction, redoAction] {
            VcsBaseSubmitEditor *editor = parameters.editorCreator();
            editor->setParameters(parameters);
            editor->registerActions(undoAction, redoAction, submitAction, diffAction);
            return editor;
        });
    }
};

void setupVcsSubmitEditor(VersionControlBase *versionControl,
                          const VcsBaseSubmitEditorParameters &parameters)
{
    auto factory = new VcsSubmitEditorFactory(versionControl, parameters);
    QObject::connect(versionControl, &QObject::destroyed, [factory] { delete factory; });
}

} // namespace VcsBase
