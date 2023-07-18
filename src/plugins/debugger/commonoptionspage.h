// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Debugger::Internal {

class SourcePathMapAspectPrivate;

// Entries starting with '(' are considered regular expressions in the ElfReader.
// This is useful when there are multiple build machines with different
// path, and the user would like to match anything up to some known
// directory to his local project.
// Syntax: (/home/.*)/KnownSubdir -> /home/my/project
using SourcePathMap = QMap<QString, QString>;

class SourcePathMapAspect : public Utils::TypedAspect<SourcePathMap>
{
public:
    SourcePathMapAspect();
    ~SourcePathMapAspect() override;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void readSettings() override;
    void writeSettings() const override;

private:
    void guiToBuffer() override;
    void bufferToGui() override;

    SourcePathMapAspectPrivate *d = nullptr;
};

class CommonSettings final : public Utils::AspectContainer
{
public:
    CommonSettings();
    ~CommonSettings();

    Utils::BoolAspect useAlternatingRowColors;
    Utils::BoolAspect useAnnotationsInMainEditor;
    Utils::BoolAspect useToolTipsInMainEditor;
    Utils::BoolAspect closeSourceBuffersOnExit;
    Utils::BoolAspect closeMemoryBuffersOnExit;
    Utils::BoolAspect raiseOnInterrupt;
    Utils::BoolAspect breakpointsFullPathByDefault;
    Utils::BoolAspect warnOnReleaseBuilds;
    Utils::IntegerAspect maximalStackDepth;

    Utils::BoolAspect fontSizeFollowsEditor;
    Utils::BoolAspect switchModeOnExit;
    Utils::BoolAspect showQmlObjectTree;
    Utils::BoolAspect stationaryEditorWhileStepping;
    Utils::BoolAspect forceLoggingToConsole;

    SourcePathMapAspect sourcePathMap;

    Utils::BoolAspect *registerForPostMortem = nullptr;
};

CommonSettings &commonSettings();

class LocalsAndExpressionsOptionsPage final : public Core::IOptionsPage
{
public:
    LocalsAndExpressionsOptionsPage();
};

} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::SourcePathMap)
