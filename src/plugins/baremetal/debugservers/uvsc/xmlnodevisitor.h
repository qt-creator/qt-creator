// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QXmlStreamAttribute>

namespace BareMetal::Gen::Xml {

class Project;
class ProjectOptions;
class Property;
class PropertyGroup;

class INodeVisitor
{
public:
    virtual ~INodeVisitor() {}

    virtual void visitProjectOptionsStart(const ProjectOptions *projectOptions)
    { Q_UNUSED(projectOptions) }
    virtual void visitProjectOptionsEnd(const ProjectOptions *projectOptions)
    { Q_UNUSED(projectOptions) }

    virtual void visitProjectStart(const Project *project) { Q_UNUSED(project) }
    virtual void visitProjectEnd(const Project *project) { Q_UNUSED(project) }

    virtual void visitPropertyStart(const Property *property) = 0;
    virtual void visitPropertyEnd(const Property *property) = 0;

    virtual void visitPropertyGroupStart(const PropertyGroup *propertyGroup) = 0;
    virtual void visitPropertyGroupEnd(const PropertyGroup *propertyGroup) = 0;
};

} // BareMetal::Gen::Xml
