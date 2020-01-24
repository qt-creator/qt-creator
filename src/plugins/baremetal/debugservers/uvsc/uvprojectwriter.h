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

#pragma once

#include "xmlprojectwriter.h"

namespace BareMetal {
namespace Internal {
namespace Uv {

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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
