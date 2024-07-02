// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildaspects.h"

#include "buildconfiguration.h"
#include "buildpropertiessettings.h"
#include "devicesupport/idevice.h"
#include "kitaspects.h"
#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <cctype>

using namespace Utils;

namespace ProjectExplorer {

class BuildDirectoryAspect::Private
{
public:
    Private(Target *target) : target(target) {}

    FilePath sourceDir;
    Target * const target;
    FilePath savedShadowBuildDir;
    QString specialProblem;
    QLabel *genericProblemSpacer;
    QLabel *specialProblemSpacer;
    QPointer<InfoLabel> genericProblemLabel;
    QPointer<InfoLabel> specialProblemLabel;
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

        const QString problem = updateProblemLabelsHelper(text);
        if (!problem.isEmpty())
            return QtFuture::makeReadyFuture(expected_str<QString>(make_unexpected(problem)));

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

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &BuildDirectoryAspect::validateInput);
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
    d->specialProblem = description;
    validateInput();
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

void BuildDirectoryAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    FilePathAspect::addToLayoutImpl(parent);
    d->genericProblemSpacer = new QLabel;
    d->specialProblemSpacer = new QLabel;
    d->genericProblemLabel = new InfoLabel({}, InfoLabel::Warning);
    d->genericProblemLabel->setElideMode(Qt::ElideNone);
    connect(d->genericProblemLabel, &QLabel::linkActivated, this, [] {
        Core::ICore::showOptionsDialog(Constants::BUILD_AND_RUN_SETTINGS_PAGE_ID);
    });
    d->specialProblemLabel = new InfoLabel({}, InfoLabel::Warning);
    d->specialProblemLabel->setElideMode(Qt::ElideNone);
    parent.addItems({Layouting::br, d->genericProblemSpacer, d->genericProblemLabel.data()});
    parent.addItems({Layouting::br, d->specialProblemSpacer, d->specialProblemLabel.data()});
    updateProblemLabels();
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

void BuildDirectoryAspect::updateProblemLabels()
{
    updateProblemLabelsHelper(value());
}

QString BuildDirectoryAspect::updateProblemLabelsHelper(const QString &value)
{
    if (!d->genericProblemLabel)
        return {};
    QTC_ASSERT(d->specialProblemLabel, return {});

    QString genericProblem;
    QString genericProblemLabelString;
    if (projectExplorerSettings().warnAgainstNonAsciiBuildDir) {
        const auto isInvalid = [](QChar c) { return c.isSpace() || !isascii(c.toLatin1()); };
        if (const auto invalidChar = Utils::findOr(value, std::nullopt, isInvalid)) {
            genericProblem = Tr::tr(
                                 "Build directory contains potentially problematic character \"%1\".")
                                 .arg(*invalidChar);
            genericProblemLabelString
                = genericProblem + " "
                  + Tr::tr("This warning can be suppressed <a href=\"dummy\">here</a>.");
        }
    }

    auto updateRow = [](const QString &text, InfoLabel *label, QLabel *spacer) {
        label->setText(text);
        label->setVisible(!text.isEmpty());
        spacer->setVisible(!text.isEmpty());
    };

    updateRow(genericProblemLabelString, d->genericProblemLabel, d->genericProblemSpacer);
    updateRow(d->specialProblem, d->specialProblemLabel, d->specialProblemSpacer);

    if (genericProblem.isEmpty() && d->specialProblem.isEmpty())
        return {};
    if (genericProblem.isEmpty())
        return d->specialProblem;
    if (d->specialProblem.isEmpty())
        return genericProblem;
    return genericProblem + '\n' + d->specialProblem;
}

SeparateDebugInfoAspect::SeparateDebugInfoAspect(AspectContainer *container)
    : TriStateAspect(container)
{
    setDisplayName(Tr::tr("Separate debug info:"));
    setSettingsKey("SeparateDebugInfo");
    setValue(buildPropertiesSettings().separateDebugInfo());
}

} // namespace ProjectExplorer
