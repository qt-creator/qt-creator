// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionsubmiteditor.h"
#include "subversionplugin.h"
#include "subversiontr.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Subversion::Internal;

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
    model->setFileStatusQualifier([](const QString &status, const QVariant &)
                                  -> VcsBase::SubmitFileModel::FileStatusHint
    {
        const QByteArray statusC = status.toLatin1();
        if (statusC == FileConflictedC)
            return VcsBase::SubmitFileModel::FileUnmerged;
        if (statusC == FileAddedC)
            return VcsBase::SubmitFileModel::FileAdded;
        if (statusC == FileModifiedC)
            return VcsBase::SubmitFileModel::FileModified;
        if (statusC == FileDeletedC)
            return VcsBase::SubmitFileModel::FileDeleted;
        return VcsBase::SubmitFileModel::FileStatusUnknown;
    } );

    for (const StatusFilePair &pair : statusOutput) {
        const VcsBase::CheckMode checkMode =
                (pair.first == QLatin1String(FileConflictedC))
                    ? VcsBase::Uncheckable
                    : VcsBase::Unchecked;
        model->addFile(pair.second, pair.first, checkMode);
    }
    setFileModel(model);
}

QByteArray SubversionSubmitEditor::fileContents() const
{
    return description().toUtf8();
}

bool SubversionSubmitEditor::setFileContents(const QByteArray &contents)
{
    setDescription(QString::fromUtf8(contents));
    return true;
}
