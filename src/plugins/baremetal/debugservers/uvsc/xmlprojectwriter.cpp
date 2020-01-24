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

#include "xmlproject.h"
#include "xmlprojectwriter.h"
#include "xmlproperty.h"
#include "xmlpropertygroup.h"

#include <ostream>

namespace BareMetal {
namespace Gen {
namespace Xml {

// ProjectWriter

ProjectWriter::ProjectWriter(std::ostream *device)
    : m_device(device)
{
    m_writer.reset(new QXmlStreamWriter(&m_buffer));
    m_writer->setAutoFormatting(true);
    m_writer->setAutoFormattingIndent(2);
}

bool ProjectWriter::write(const Project *project)
{
    m_buffer.clear();
    m_writer->writeStartDocument();
    project->accept(this);
    m_writer->writeEndDocument();
    if (m_writer->hasError())
        return false;
    m_device->write(&*std::cbegin(m_buffer), m_buffer.size());
    return m_device->good();
}

void ProjectWriter::visitPropertyStart(const Property *property)
{
    const QString value = property->value().toString();
    const QString name = QString::fromUtf8(property->name());
    m_writer->writeTextElement(name, value);
}

void ProjectWriter::visitPropertyEnd(const Property *property)
{
    Q_UNUSED(property)
}

void ProjectWriter::visitPropertyGroupStart(const PropertyGroup *propertyGroup)
{
    const QString name = QString::fromUtf8(propertyGroup->name());
    m_writer->writeStartElement(name);
}

void ProjectWriter::visitPropertyGroupEnd(const PropertyGroup *propertyGroup)
{
    Q_UNUSED(propertyGroup)
    m_writer->writeEndElement();
}

QXmlStreamWriter *ProjectWriter::writer() const
{
    return m_writer.get();
}

// ProjectOptionsWriter

ProjectOptionsWriter::ProjectOptionsWriter(std::ostream *device)
    : m_device(device)
{
    m_writer.reset(new QXmlStreamWriter(&m_buffer));
    m_writer->setAutoFormatting(true);
    m_writer->setAutoFormattingIndent(2);
}

bool ProjectOptionsWriter::write(const ProjectOptions *projectOptions)
{
    m_buffer.clear();
    m_writer->writeStartDocument();
    projectOptions->accept(this);
    m_writer->writeEndDocument();
    if (m_writer->hasError())
        return false;
    m_device->write(&*std::cbegin(m_buffer), m_buffer.size());
    return m_device->good();
}

void ProjectOptionsWriter::visitPropertyStart(const Property *property)
{
    const QString value = property->value().toString();
    const QString name = QString::fromUtf8(property->name());
    m_writer->writeTextElement(name, value);
}

void ProjectOptionsWriter::visitPropertyEnd(const Property *property)
{
    Q_UNUSED(property)
}

void ProjectOptionsWriter::visitPropertyGroupStart(const PropertyGroup *propertyGroup)
{
    const QString name = QString::fromUtf8(propertyGroup->name());
    m_writer->writeStartElement(name);
}

void ProjectOptionsWriter::visitPropertyGroupEnd(const PropertyGroup *propertyGroup)
{
    Q_UNUSED(propertyGroup)
    m_writer->writeEndElement();
}

QXmlStreamWriter *ProjectOptionsWriter::writer() const
{
    return m_writer.get();
}

} // namespace Xml
} // namespace Gen
} // namespace BareMetal
