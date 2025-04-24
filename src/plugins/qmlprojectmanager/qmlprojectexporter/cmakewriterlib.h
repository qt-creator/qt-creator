// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "cmakewriterv1.h"

namespace QmlProjectManager {

namespace QmlProjectExporter {

class CMakeWriterLib final : public CMakeWriterV1
{
public:
    CMakeWriterLib(CMakeGenerator *parent);

    QString mainLibName() const override;
    void transformNode(NodePtr &node) const override;

    int identifier() const override;
    void writeRootCMakeFile(const NodePtr &node) const override;
    void writeModuleCMakeFile(const NodePtr &node, const NodePtr &root) const override;
    void writeSourceFiles(const NodePtr &node, const NodePtr &root) const override;
};

} // namespace QmlProjectExporter
} // namespace QmlProjectManager
