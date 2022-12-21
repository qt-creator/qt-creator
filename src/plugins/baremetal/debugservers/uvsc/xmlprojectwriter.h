// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlnodevisitor.h"

#include <memory>

namespace BareMetal::Gen::Xml {

// ProjectWriter

class ProjectWriter : public INodeVisitor
{
    Q_DISABLE_COPY(ProjectWriter)
public:
    explicit ProjectWriter(std::ostream *device);
    bool write(const Project *project);

protected:
    QXmlStreamWriter *writer() const;

private:
    void visitPropertyStart(const Property *property) final;
    void visitPropertyEnd(const Property *property) final;

    void visitPropertyGroupStart(const PropertyGroup *propertyGroup) final;
    void visitPropertyGroupEnd(const PropertyGroup *propertyGroup) final;

    std::ostream *m_device = nullptr;
    QByteArray m_buffer;
    std::unique_ptr<QXmlStreamWriter> m_writer;
};

// ProjectOptionsWriter

class ProjectOptionsWriter : public INodeVisitor
{
    Q_DISABLE_COPY(ProjectOptionsWriter)
public:
    explicit ProjectOptionsWriter(std::ostream *device);
    bool write(const ProjectOptions *projectOptions);

protected:
    QXmlStreamWriter *writer() const;

private:
    void visitPropertyStart(const Property *property) final;
    void visitPropertyEnd(const Property *property) final;

    void visitPropertyGroupStart(const PropertyGroup *propertyGroup) final;
    void visitPropertyGroupEnd(const PropertyGroup *propertyGroup) final;

    std::ostream *m_device = nullptr;
    QByteArray m_buffer;
    std::unique_ptr<QXmlStreamWriter> m_writer;
};

} // BareMetal::Gen::Xml
