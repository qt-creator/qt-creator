// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copystep.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/aspects.h>

using namespace Utils;

namespace ProjectExplorer::Internal {

const char SOURCE_KEY[] = "ProjectExplorer.CopyStep.Source";
const char TARGET_KEY[] = "ProjectExplorer.CopyStep.Target";

class CopyStepBase : public BuildStep
{
public:
    CopyStepBase(BuildStepList *bsl, Id id)
        : BuildStep(bsl, id)
    {
        m_sourceAspect = addAspect<StringAspect>();
        m_sourceAspect->setSettingsKey(SOURCE_KEY);
        m_sourceAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
        m_sourceAspect->setLabelText(Tr::tr("Source:"));

        m_targetAspect = addAspect<StringAspect>();
        m_targetAspect->setSettingsKey(TARGET_KEY);
        m_targetAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
        m_targetAspect->setLabelText(Tr::tr("Target:"));

        addMacroExpander();
    }

protected:
    bool init() final
    {
        m_source = m_sourceAspect->filePath();
        m_target = m_targetAspect->filePath();
        return m_source.exists();
    }

    void doRun() final
    {
        // FIXME: asyncCopy does not handle directories yet.
        QTC_ASSERT(m_source.isFile(), emit finished(false));
        m_source.asyncCopy(m_target, this, [this](const expected_str<void> &cont) {
            if (!cont) {
                addOutput(cont.error(), OutputFormat::ErrorMessage);
                addOutput(Tr::tr("Copying failed"), OutputFormat::ErrorMessage);
                emit finished(false);
            } else {
                addOutput(Tr::tr("Copying finished"), OutputFormat::NormalMessage);
                emit finished(true);
            }
        });
    }

    StringAspect *m_sourceAspect;
    StringAspect *m_targetAspect;

private:
    FilePath m_source;
    FilePath m_target;
};

class CopyFileStep final : public CopyStepBase
{
public:
    CopyFileStep(BuildStepList *bsl, Id id)
        : CopyStepBase(bsl, id)
     {
        m_sourceAspect->setExpectedKind(PathChooser::File);
        m_targetAspect->setExpectedKind(PathChooser::SaveFile);

        setSummaryUpdater([] {
            return QString("<b>" + Tr::tr("Copy file") + "</b>");
        });
     }
};

class CopyDirectoryStep final : public CopyStepBase
{
public:
    CopyDirectoryStep(BuildStepList *bsl, Id id)
        : CopyStepBase(bsl, id)
     {
        m_sourceAspect->setExpectedKind(PathChooser::Directory);
        m_targetAspect->setExpectedKind(PathChooser::Directory);

        setSummaryUpdater([] {
            return QString("<b>" + Tr::tr("Copy directory recursively") + "</b>");
        });
     }
};

// Factories

CopyFileStepFactory::CopyFileStepFactory()
{
    registerStep<CopyFileStep>(Constants::COPY_FILE_STEP);
    //: Default CopyStep display name
    setDisplayName(Tr::tr("Copy file"));
}

CopyDirectoryStepFactory::CopyDirectoryStepFactory()
{
    registerStep<CopyDirectoryStep>(Constants::COPY_DIRECTORY_STEP);
    //: Default CopyStep display name
    setDisplayName(Tr::tr("Copy directory recursively"));
}

} // ProjectExplorer::Internal
