// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildaspects.h"

#include "buildconfiguration.h"
#include "buildpropertiessettings.h"
#include "devicesupport/idevice.h"
#include "kitaspects.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/fileutils.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

using namespace Utils;

namespace ProjectExplorer {

class BuildDirectoryAspect::Private
{
public:
    Private(Target *target) : target(target) {}

    FilePath sourceDir;
    Target * const target;
    FilePath savedShadowBuildDir;
    QString problem;
    QPointer<InfoLabel> problemLabel;
};

BuildDirectoryAspect::BuildDirectoryAspect(AspectContainer *container, const BuildConfiguration *bc)
    : FilePathAspect(container),
      d(new Private(bc->target()))
{
    setSettingsKey("ProjectExplorer.BuildConfiguration.BuildDirectory");
    setLabelText(Tr::tr("Build directory:"));
    setExpectedKind(Utils::PathChooser::Directory);

    setValidationFunction([this](QString text) -> FancyLineEdit::AsyncValidationFuture {
        const FilePath fixedDir = fixupDir(FilePath::fromUserInput(text));
        if (!fixedDir.isEmpty())
            text = fixedDir.toUserOutput();

        const FilePath newPath = FilePath::fromUserInput(text);
        const auto buildDevice = BuildDeviceKitAspect::device(d->target->kit());

        if (buildDevice && buildDevice->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE
            && !buildDevice->rootPath().ensureReachable(newPath)) {
            return QtFuture::makeReadyFuture((Utils::expected_str<QString>(make_unexpected(
                Tr::tr("The build directory is not reachable from the build device.")))));
        }
        return pathChooser()->defaultValidationFunction()(text);
    });

    setOpenTerminalHandler([this, bc] {
        Core::FileUtils::openTerminal(expandedValue(), bc->environment());
    });
}

BuildDirectoryAspect::~BuildDirectoryAspect()
{
    delete d;
}

void BuildDirectoryAspect::allowInSourceBuilds(const FilePath &sourceDir)
{
    d->sourceDir = sourceDir;
    makeCheckable(CheckBoxPlacement::Top, Tr::tr("Shadow build:"), Key());
    setChecked(d->sourceDir != expandedValue());
}

bool BuildDirectoryAspect::isShadowBuild() const
{
    return !d->sourceDir.isEmpty() && d->sourceDir != expandedValue();
}

void BuildDirectoryAspect::setProblem(const QString &description)
{
    d->problem = description;
    updateProblemLabel();
}

void BuildDirectoryAspect::toMap(Store &map) const
{
    FilePathAspect::toMap(map);
    if (!d->sourceDir.isEmpty()) {
        const FilePath shadowDir = isChecked() ? expandedValue() : d->savedShadowBuildDir;
        saveToMap(map, shadowDir.toSettings(), QString(), settingsKey() + ".shadowDir");
    }
}

void BuildDirectoryAspect::fromMap(const Store &map)
{
    FilePathAspect::fromMap(map);
    if (!d->sourceDir.isEmpty()) {
        d->savedShadowBuildDir = FilePath::fromSettings(map.value(settingsKey() + ".shadowDir"));
        if (d->savedShadowBuildDir.isEmpty())
            setValue(d->sourceDir);
        setChecked(d->sourceDir != expandedValue()); // FIXME: Check.
    }
}

void BuildDirectoryAspect::addToLayout(Layouting::LayoutItem &parent)
{
    FilePathAspect::addToLayout(parent);
    d->problemLabel = new InfoLabel({}, InfoLabel::Warning);
    d->problemLabel->setElideMode(Qt::ElideNone);
    parent.addItems({Layouting::br, Layouting::empty, d->problemLabel.data()});
    updateProblemLabel();
    if (!d->sourceDir.isEmpty()) {
        connect(this, &StringAspect::checkedChanged, this, [this] {
            if (isChecked()) {
                setValue(d->savedShadowBuildDir.isEmpty()
                            ? d->sourceDir : d->savedShadowBuildDir);
            } else {
                d->savedShadowBuildDir = expandedValue(); // FIXME: Check.
                setValue(d->sourceDir);
            }
        });
    }

    const auto buildDevice = DeviceKitAspect::device(d->target->kit());
    if (buildDevice && buildDevice->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        pathChooser()->setAllowPathFromDevice(true);
    else
        pathChooser()->setAllowPathFromDevice(false);
}

FilePath BuildDirectoryAspect::fixupDir(const FilePath &dir)
{
    if (dir.needsDevice())
        return dir;
    if (HostOsInfo::isWindowsHost() && !dir.startsWithDriveLetter())
        return {};
    const QString dirString = dir.toString().toLower();
    const QStringList drives = Utils::transform(QDir::drives(), [](const QFileInfo &fi) {
        return fi.absoluteFilePath().toLower().chopped(1);
    });
    if (!Utils::contains(drives, [&dirString](const QString &drive) {
            return dirString.startsWith(drive);
        }) && !drives.isEmpty()) {
        QString newDir = dir.path();
        newDir.replace(0, 2, drives.first());
        return dir.withNewPath(newDir);
    }
    return {};
}

void BuildDirectoryAspect::updateProblemLabel()
{
    if (!d->problemLabel)
        return;

    d->problemLabel->setText(d->problem);
    d->problemLabel->setVisible(!d->problem.isEmpty());
}

SeparateDebugInfoAspect::SeparateDebugInfoAspect(AspectContainer *container)
    : TriStateAspect(container)
{
    setDisplayName(Tr::tr("Separate debug info:"));
    setSettingsKey("SeparateDebugInfo");
    setValue(buildPropertiesSettings().separateDebugInfo());
}

} // namespace ProjectExplorer
