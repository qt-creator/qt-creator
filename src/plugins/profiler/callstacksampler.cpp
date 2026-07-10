// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callstacksampler.h"

#include "macsampler.h"
#include "processpickerdialog.h"

#include "profilertr.h"

#include <utils/layoutbuilder.h>
#include <utils/processinfo.h>
#include <utils/qtdesignwidgets.h>

#include <QtTaskTree/QThreadFunction>

#include <memory>
#include <optional>

using namespace Profiler;
using namespace QtTaskTree;
using namespace Utils;

namespace QmlProfiler::Internal {

CallStackSamplerSettings::CallStackSamplerSettings()
{
    setSettingsGroup("CallStackSampler");

    intervalUs.setSettingsKey("IntervalUs");
    intervalUs.setLabelText(Tr::tr("Sample interval (µs):"));
    intervalUs.setRange(0, 1000000); // 0 = as fast as possible.
    intervalUs.setDefaultValue(200);

    attach.setSettingsKey("Attach");
    attach.setLabel(Tr::tr("Attach to a running process"),
                    BoolAspect::LabelPlacement::AtCheckBox);
    // The launch settings are irrelevant while attaching.
    const auto updateLaunchEnabled = [this] {
        const bool launching = !attach();
        executable.setEnabled(launching);
        arguments.setEnabled(launching);
        workingDirectory.setEnabled(launching);
    };
    updateLaunchEnabled();
    connect(&attach, &BoolAspect::changed, this, updateLaunchEnabled);

    setLayouter([this] {
        using namespace Layouting;
        auto pick = new QtcButton(Tr::tr("Select Process…"), QtcButton::SmallSecondary);
        auto picked = new QtcLabel(m_pickedName.isEmpty() ? Tr::tr("No process selected")
                                                          : m_pickedName,
                                   QtcLabel::Secondary);
        pick->setEnabled(attach());
        connect(&attach, &BoolAspect::changed, pick, [this, pick] { pick->setEnabled(attach()); });
        connect(pick, &QAbstractButton::clicked, this, [this, picked] {
            const std::optional<ProcessInfo> info = ProcessPickerDialog::pickProcess();
            if (!info)
                return;
            m_pickedPid = info->processId;
            m_pickedName = FilePath::fromUserInput(info->executable).fileName();
            picked->setText(m_pickedName);
        });
        return Column {
            executable,
            arguments,
            workingDirectory,
            Row { intervalUs, st },
            Row { attach, pick, picked, st },
        };
    });
}

Result<std::shared_ptr<RecordingSession>> CallStackSamplerSettings::createSession() const
{
    auto session = std::make_shared<RecordingSession>();
    session->intervalUs = int(intervalUs());
    if (attach()) {
        if (m_pickedPid == 0)
            return ResultError(Tr::tr("Select a process to attach to."));
        session->pid = m_pickedPid;
        session->processName = m_pickedName;
        return session;
    }
    if (Result<> launch = fillLaunch(*session); !launch)
        return ResultError(launch.error());
    return session;
}

CallStackSampler::CallStackSampler()
    : m_settings(std::make_unique<CallStackSamplerSettings>())
{}

CallStackSampler::~CallStackSampler() = default;

QString CallStackSampler::displayName() const
{
    return Tr::tr("Call-Stack Sampler");
}

bool CallStackSampler::isAvailable(QString *error) const
{
#ifdef Q_OS_MACOS
    Q_UNUSED(error)
    return true;
#else
    if (error)
        *error = Tr::tr("Call-stack sampling is only implemented on macOS.");
    return false;
#endif
}

SamplerSettings *CallStackSampler::settings() const
{
    return m_settings.get();
}

ExecutableItem CallStackSampler::captureRecipe(const std::shared_ptr<RecordingSession> &session) const
{
    // The cadence comes from the session (set by createSession, or by a composite
    // backend that drives this capture), so the sampling worker can read it.
    const int intervalUs = session->intervalUs;

    const auto onSetup = [session, intervalUs](QThreadFunction<Result<FilePath>> &sampling) {
        // recordSampleTrace blocks until session->stop is set, so it runs on a
        // worker thread; the target stays alive (the launched process is only
        // terminated once this task finishes) so it can still be symbolized.
        sampling.setThreadFunctionData([session, intervalUs] {
            SamplerOptions opts;
            opts.pid = session->pid.load();
            opts.processName = session->processName;
            opts.intervalUs = intervalUs;
            session->markStarted(); // capture is live; the duration clock can start
            return recordSampleTrace(opts, session->stop, &session->progress);
        });
    };
    const auto onDone = [session](const QThreadFunction<Result<FilePath>> &sampling,
                                  DoneWith result) {
        if (result != DoneWith::Cancel)
            session->result = sampling.result();
    };
    return QThreadFunctionTask<Result<FilePath>>(onSetup, onDone);
}

} // namespace QmlProfiler::Internal
