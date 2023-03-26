// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

#include <QDialog>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Squish {
namespace Internal {

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
    Utils::IntegerAspect autTimeout;
    Utils::IntegerAspect responseTimeout;
    Utils::IntegerAspect postMortemWaitTime;
    Utils::BoolAspect animatedCursor;
};

class SquishSettings : public Utils::AspectContainer
{
    Q_OBJECT
public:
    SquishSettings();

    Utils::FilePath scriptsPath(Language language) const;

    Utils::StringAspect squishPath;
    Utils::StringAspect licensePath;
    Utils::StringAspect serverHost;
    Utils::IntegerAspect serverPort;
    Utils::BoolAspect local;
    Utils::BoolAspect verbose;
    Utils::BoolAspect minimizeIDE;

signals:
    void squishPathChanged();
};

class SquishSettingsPage final : public Core::IOptionsPage
{
public:
    SquishSettingsPage(SquishSettings *settings);
};

class SquishServerSettingsDialog : public QDialog
{
public:
    explicit SquishServerSettingsDialog(QWidget *parent = nullptr);

private:
    void configWriteFailed(QProcess::ProcessError error);
};

} // namespace Internal
} // namespace Squish
