// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfconfigwidget.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"
#include "perfsettings.h"

#include <coreplugin/icore.h>

#include <QSettings>

#include <utils/qtcprocess.h>

using namespace Utils;

namespace PerfProfiler {

PerfSettings::PerfSettings(ProjectExplorer::Target *target)
{
    setConfigWidgetCreator([this, target] {
        auto widget = new Internal::PerfConfigWidget(this);
        widget->setTracePointsButtonVisible(target != nullptr);
        widget->setTarget(target);
        return widget;
    });

    registerAspect(&period);
    period.setSettingsKey("Analyzer.Perf.Frequency");
    period.setRange(250, 2147483647);
    period.setDefaultValue(250);
    period.setLabelText(Tr::tr("Sample period:"));

    registerAspect(&stackSize);
    stackSize.setSettingsKey("Analyzer.Perf.StackSize");
    stackSize.setRange(4096, 65536);
    stackSize.setDefaultValue(4096);
    stackSize.setLabelText(Tr::tr("Stack snapshot size (kB):"));

    registerAspect(&sampleMode);
    sampleMode.setSettingsKey("Analyzer.Perf.SampleMode");
    sampleMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    sampleMode.setLabelText(Tr::tr("Sample mode:"));
    sampleMode.addOption({Tr::tr("frequency (Hz)"), {}, QString("-F")});
    sampleMode.addOption({Tr::tr("event count"), {}, QString("-c")});
    sampleMode.setDefaultValue(0);

    registerAspect(&callgraphMode);
    callgraphMode.setSettingsKey("Analyzer.Perf.CallgraphMode");
    callgraphMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    callgraphMode.setLabelText(Tr::tr("Call graph mode:"));
    callgraphMode.addOption({Tr::tr("dwarf"), {}, QString(Constants::PerfCallgraphDwarf)});
    callgraphMode.addOption({Tr::tr("frame pointer"), {}, QString("fp")});
    callgraphMode.addOption({Tr::tr("last branch record"), {}, QString("lbr")});
    callgraphMode.setDefaultValue(0);

    registerAspect(&events);
    events.setSettingsKey("Analyzer.Perf.Events");
    events.setDefaultValue({"cpu-cycles"});

    registerAspect(&extraArguments);
    extraArguments.setSettingsKey("Analyzer.Perf.ExtraArguments");
    extraArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    extraArguments.setLabelText(Tr::tr("Additional arguments:"));
    extraArguments.setSpan(4);

    connect(&callgraphMode, &SelectionAspect::volatileValueChanged, this, [this](int index) {
        stackSize.setEnabled(index == 0);
    });

    connect(this, &AspectContainer::fromMapFinished, this, &PerfSettings::changed);

    readGlobalSettings();
}

PerfSettings::~PerfSettings()
{
}

void PerfSettings::readGlobalSettings()
{
    QVariantMap defaults;

    // Read stored values
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::AnalyzerSettingsGroupId));
    QVariantMap map = defaults;
    for (QVariantMap::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    fromMap(map);
}

void PerfSettings::writeGlobalSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::AnalyzerSettingsGroupId));
    QVariantMap map;
    toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
}

QStringList PerfSettings::perfRecordArguments() const
{
    QString callgraphArg = callgraphMode.itemValue().toString();
    if (callgraphArg == Constants::PerfCallgraphDwarf)
        callgraphArg += "," + QString::number(stackSize.value());

    QString events;
    for (const QString &event : this->events.value()) {
        if (!event.isEmpty()) {
            if (!events.isEmpty())
                events.append(',');
            events.append(event);
        }
    }

    return QStringList({"-e", events,
                        "--call-graph", callgraphArg,
                        sampleMode.itemValue().toString(),
                        QString::number(period.value())})
         + ProcessArgs::splitArgs(extraArguments.value());
}

void PerfSettings::resetToDefault()
{
    PerfSettings defaults;
    QVariantMap map;
    defaults.toMap(map);
    fromMap(map);
}

} // namespace PerfProfiler
