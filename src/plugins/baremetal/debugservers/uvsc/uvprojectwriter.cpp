// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvprojectwriter.h"

namespace BareMetal::Internal::Uv {

// ProjectWriter

ProjectWriter::ProjectWriter(std::ostream *device)
    : Gen::Xml::ProjectWriter(device)
{
}

void ProjectWriter::visitProjectStart(const Gen::Xml::Project *project)
{
    Q_UNUSED(project)
    writer()->writeStartElement("Project");
    writer()->writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    writer()->writeAttribute("xsi:noNamespaceSchemaLocation", "project_proj.xsd");
}

void ProjectWriter::visitProjectEnd(const Gen::Xml::Project *project)
{
    Q_UNUSED(project)
    writer()->writeEndElement();
}

// ProjectOptionsWriter

ProjectOptionsWriter::ProjectOptionsWriter(std::ostream *device)
    : Gen::Xml::ProjectOptionsWriter(device)
{
}

void ProjectOptionsWriter::visitProjectOptionsStart(
        const Gen::Xml::ProjectOptions *projectOptions)
{
    Q_UNUSED(projectOptions)
    writer()->writeStartElement("ProjectOpt");
    writer()->writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    writer()->writeAttribute("xsi:noNamespaceSchemaLocation", "project_optx.xsd");
}

void ProjectOptionsWriter::visitProjectOptionsEnd(
        const Gen::Xml::ProjectOptions *projectOptions)
{
    Q_UNUSED(projectOptions)
    writer()->writeEndElement();
}

} // BareMetal::Internal::Uv
