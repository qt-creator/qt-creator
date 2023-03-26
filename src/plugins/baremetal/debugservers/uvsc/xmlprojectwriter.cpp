// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmlproject.h"
#include "xmlprojectwriter.h"
#include "xmlproperty.h"
#include "xmlpropertygroup.h"

#include <ostream>

namespace BareMetal::Gen::Xml {

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

} // BareMetal::Gen::Xml
