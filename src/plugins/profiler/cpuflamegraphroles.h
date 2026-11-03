// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <Qt>

namespace Profiler::Internal::CpuFlameGraph {

// The item data roles CpuFlameGraphView reads. A model of sampled call stacks
// answers them for every node, and for the invalid index as the invisible
// root, whose SamplesRole is the total. A role the model has no value for is
// left out of the details.
enum Role {
    TypeIdRole = Qt::UserRole + 1, // the node's function, for selecting it
    DisplayNameRole,               // the title of the node's details
    FunctionRole,
    SamplesRole,
    SelfRole,
    SourceFileRole,
    SourceLineRole,
    BinaryRole,
    ResourcePeakRole,
    ResourceAllocationsRole,
    ResourceReleasesRole,
    FirstModelSpecificRole
};

} // namespace Profiler::Internal::CpuFlameGraph
