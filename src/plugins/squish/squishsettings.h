// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QDialog>
#include <QProcess>

namespace Squish::Internal {

enum class Language;

class SquishServerSettings : public Utils::AspectContainer
{
public:
    SquishServerSettings();

    void setFromXmlOutput(const QString &output);

    QMap<QString, QString> mappedAuts; // name, path
    QMap<QString, QString> attachableAuts; // name, host:port
    QStringList autPaths; // absolute path
    QStringList licensedToolkits;
    Utils::IntegerAspect autTimeout{this};
    Utils::IntegerAspect responseTimeout{this};
    Utils::IntegerAspect postMortemWaitTime{this};
    Utils::BoolAspect animatedCursor{this};
};

class SquishSettings : public Utils::AspectContainer
{
public:
    SquishSettings();

    Utils::FilePath scriptsPath(Language language) const;

    Utils::FilePathAspect squishPath{this};
    Utils::FilePathAspect licensePath{this};
    Utils::StringAspect serverHost{this};
    Utils::IntegerAspect serverPort{this};
    Utils::BoolAspect local{this};
    Utils::BoolAspect verbose{this};
    Utils::BoolAspect minimizeIDE{this};
};

SquishSettings &settings();

class SquishServerSettingsDialog : public QDialog
{
public:
    explicit SquishServerSettingsDialog(QWidget *parent = nullptr);

private:
    void configWriteFailed(QProcess::ProcessError error);
};

} // Squish::Internal
