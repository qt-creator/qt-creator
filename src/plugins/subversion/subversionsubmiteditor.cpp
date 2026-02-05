// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionsubmiteditor.h"
#include "subversionplugin.h"
#include "subversiontr.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Utils;

namespace Subversion::Internal {

SubversionSubmitEditor::SubversionSubmitEditor() :
    VcsBase::VcsBaseSubmitEditor(new VcsBase::SubmitEditorWidget)
{
    document()->setPreferredDisplayName(Tr::tr("Subversion Submit"));
    setDescriptionMandatory(false);
}

void SubversionSubmitEditor::setStatusList(const QList<StatusFilePair> &statusOutput)
{
    auto model = new VcsBase::SubmitFileModel(this);
    // Hack to allow completion in "description" field : completion needs a root repository, the
    // checkScriptWorkingDirectory property is fine (at this point it was set by SubversionPlugin)
    model->setRepositoryRoot(checkScriptWorkingDirectory());
    model->setFileStatusQualifier([](const QString & , const QVariant &extraData) {
        const int status = extraData.toInt();
        switch (status) {
        case FileConflictedC:
            return Core::VcsFileState::Unmerged;
        case FileUntrackedC:
            return Core::VcsFileState::Untracked;
        case FileAddedC:
            return Core::VcsFileState::Added;
        case FileModifiedC:
            return Core::VcsFileState::Modified;
        case FileDeletedC:
            return Core::VcsFileState::Deleted;
        default:
            return Core::VcsFileState::Unknown;
        }
    });

    auto statusText = [](char status) {
        switch (status) {
        case FileConflictedC:
            return Tr::tr("conflicted");
        case FileUntrackedC:
            return Tr::tr("untracked");
        case FileAddedC:
            return Tr::tr("added");
        case FileModifiedC:
            return Tr::tr("modified");
        case FileDeletedC:
            return Tr::tr("deleted");
        default:
            return Tr::tr("unknown");
        }
    };

    for (const StatusFilePair &pair : statusOutput) {
        const VcsBase::CheckMode checkMode =
                (pair.first == FileConflictedC)
                    ? VcsBase::Uncheckable
                    : VcsBase::Unchecked;
        model->addFile(pair.second, statusText(pair.first), checkMode,
                       QVariant(static_cast<int>(pair.first)));
    }
    setFileModel(model);
}

QByteArray SubversionSubmitEditor::fileContents() const
{
    return description().toUtf8();
}

Result<> SubversionSubmitEditor::setFileContents(const QByteArray &contents)
{
    setDescription(QString::fromUtf8(contents));
    return ResultOk;
}

} // namespace Subversion::Internal
