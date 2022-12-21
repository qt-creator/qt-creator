// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlproject.h"
#include "xmlpropertygroup.h"

#include <utils/fileutils.h>

namespace Debugger { class DebuggerRunTool; }

namespace BareMetal::Internal {

class UvscServerProvider;

namespace Uv {

class DeviceSelection;

// Helper function.
QString buildPackageId(const DeviceSelection &selection);

// UvProject

class Project final : public Gen::Xml::Project
{
public:
    explicit Project(const UvscServerProvider *provider, Debugger::DebuggerRunTool *runTool);

protected:
    Gen::Xml::PropertyGroup *m_target = nullptr;

private:
    void fillAllFiles(const Utils::FilePaths &headers, const Utils::FilePaths &sources,
                      const Utils::FilePaths &assemblers);
};

// ProjectOptions

class ProjectOptions : public Gen::Xml::ProjectOptions
{
public:
    explicit ProjectOptions(const UvscServerProvider *provider);

protected:
    Gen::Xml::PropertyGroup *m_targetOption = nullptr;
    Gen::Xml::PropertyGroup *m_debugOpt = nullptr;
};

} // Uv

} // BareMetal::Internal
