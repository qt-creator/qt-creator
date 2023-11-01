// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

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
    explicit SourcePathMapAspect(Utils::AspectContainer *container);
    ~SourcePathMapAspect() override;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void addToLayout(Layouting::LayoutItem &parent) override;

    void readSettings() override;
    void writeSettings() const override;

    bool isDirty() override;

private:
    bool guiToBuffer() override;
    void bufferToGui() override;

    SourcePathMapAspectPrivate *d = nullptr;
};

class CommonSettings final : public Utils::AspectContainer
{
public:
    CommonSettings();
    ~CommonSettings();

    Utils::BoolAspect useAlternatingRowColors{this};
    Utils::BoolAspect useAnnotationsInMainEditor{this};
    Utils::BoolAspect useToolTipsInMainEditor{this};
    Utils::BoolAspect closeSourceBuffersOnExit{this};
    Utils::BoolAspect closeMemoryBuffersOnExit{this};
    Utils::BoolAspect raiseOnInterrupt{this};
    Utils::BoolAspect breakpointsFullPathByDefault{this};
    Utils::BoolAspect warnOnReleaseBuilds{this};
    Utils::IntegerAspect maximalStackDepth{this};

    Utils::BoolAspect fontSizeFollowsEditor{this};
    Utils::BoolAspect switchModeOnExit{this};
    Utils::BoolAspect showQmlObjectTree{this};
    Utils::BoolAspect stationaryEditorWhileStepping{this};
    Utils::BoolAspect forceLoggingToConsole{this};

    SourcePathMapAspect sourcePathMap{this};

    Utils::BoolAspect *registerForPostMortem = nullptr;
};

CommonSettings &commonSettings();


class LocalsAndExpressionsSettings final : public Utils::AspectContainer
{
public:
    LocalsAndExpressionsSettings();

    Utils::BoolAspect useDebuggingHelpers{this};
    Utils::BoolAspect useCodeModel{this};
    Utils::BoolAspect showThreadNames{this};
    Utils::FilePathAspect extraDumperFile{this};   // For loading a file. Recommended.
    Utils::StringAspect extraDumperCommands{this}; // To modify an existing setup.

    Utils::BoolAspect showStdNamespace{this};
    Utils::BoolAspect showQtNamespace{this};
    Utils::BoolAspect showQObjectNames{this};

    Utils::IntegerAspect maximalStringLength{this};
    Utils::IntegerAspect displayStringLimit{this};
    Utils::IntegerAspect defaultArraySize{this};
};

LocalsAndExpressionsSettings &localsAndExpressionSettings();


} // Debugger::Internal

Q_DECLARE_METATYPE(Debugger::Internal::SourcePathMap)
