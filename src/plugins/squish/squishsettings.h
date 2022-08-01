// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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

class SquishSettings : public Utils::AspectContainer
{
public:
    SquishSettings();

    Utils::StringAspect squishPath;
    Utils::StringAspect licensePath;
    Utils::StringAspect serverHost;
    Utils::IntegerAspect serverPort;
    Utils::BoolAspect local;
    Utils::BoolAspect verbose;
    Utils::BoolAspect minimizeIDE;
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
