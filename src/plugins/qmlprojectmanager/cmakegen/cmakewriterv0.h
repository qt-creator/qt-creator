// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "cmakewriter.h"

namespace QmlProjectManager {

namespace GenerateCmake {

class CMakeWriterV0 final : public CMakeWriter
{
public:
    CMakeWriterV0(CMakeGenerator *parent);

    bool isPlugin(const NodePtr &node) const override;
    void transformNode(NodePtr &node) const override;

    void writeRootCMakeFile(const NodePtr &node) const override;
    void writeModuleCMakeFile(const NodePtr &node, const NodePtr &root) const override;
    void writeSourceFiles(const NodePtr &node, const NodePtr &root) const override;
};

} // namespace GenerateCmake
} // namespace QmlProjectManager
