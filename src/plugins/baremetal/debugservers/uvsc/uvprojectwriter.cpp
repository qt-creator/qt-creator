/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "uvprojectwriter.h"

namespace BareMetal {
namespace Internal {
namespace Uv {

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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
