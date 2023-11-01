// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "copystep.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/aspects.h>
#include <utils/filestreamer.h>

using namespace Tasking;
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
        m_sourceAspect.setSettingsKey(SOURCE_KEY);
        m_sourceAspect.setLabelText(Tr::tr("Source:"));

        m_targetAspect.setSettingsKey(TARGET_KEY);
        m_targetAspect.setLabelText(Tr::tr("Target:"));

        addMacroExpander();
    }

protected:
    bool init() final
    {
        m_source = m_sourceAspect();
        m_target = m_targetAspect();
        return m_source.exists();
    }

    FilePathAspect m_sourceAspect{this};
    FilePathAspect m_targetAspect{this};

private:
    GroupItem runRecipe() final
    {
        const auto onSetup = [this](FileStreamer &streamer) {
            QTC_ASSERT(m_source.isFile(), return SetupResult::StopWithError);
            streamer.setSource(m_source);
            streamer.setDestination(m_target);
            return SetupResult::Continue;
        };
        const auto onDone = [this](const FileStreamer &) {
            addOutput(Tr::tr("Copying finished."), OutputFormat::NormalMessage);
        };
        const auto onError = [this](const FileStreamer &) {
            addOutput(Tr::tr("Copying failed."), OutputFormat::ErrorMessage);
        };
        return FileStreamerTask(onSetup, onDone, onError);
    }

    FilePath m_source;
    FilePath m_target;
};

class CopyFileStep final : public CopyStepBase
{
public:
    CopyFileStep(BuildStepList *bsl, Id id)
        : CopyStepBase(bsl, id)
     {
        // Expected kind could be stricter in theory, but since this here is
        // a last stand fallback, better not impose extra "nice to have"
        // work on the system.
        m_sourceAspect.setExpectedKind(PathChooser::Any); // "File"
        m_targetAspect.setExpectedKind(PathChooser::Any); // "SaveFile"

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
        m_sourceAspect.setExpectedKind(PathChooser::Directory);
        m_targetAspect.setExpectedKind(PathChooser::Directory);

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
