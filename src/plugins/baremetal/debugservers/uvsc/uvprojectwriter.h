// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlprojectwriter.h"

namespace BareMetal::Internal::Uv {

// ProjectWriter

class ProjectWriter final : public Gen::Xml::ProjectWriter
{
    Q_DISABLE_COPY(ProjectWriter)
public:
    explicit ProjectWriter(std::ostream *device);

private:
    void visitProjectStart(const Gen::Xml::Project *project) final;
    void visitProjectEnd(const Gen::Xml::Project *project) final;
};

// ProjectOptionsWriter

class ProjectOptionsWriter final : public Gen::Xml::ProjectOptionsWriter
{
    Q_DISABLE_COPY(ProjectOptionsWriter)
public:
    explicit ProjectOptionsWriter(std::ostream *device);

private:
    void visitProjectOptionsStart(const Gen::Xml::ProjectOptions *projectOptions) final;
    void visitProjectOptionsEnd(const Gen::Xml::ProjectOptions *projectOptions) final;
};

} // BareMetal::Internal::Uv
