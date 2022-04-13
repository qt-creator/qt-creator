/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qtsupport_global.h"

#include <utils/aspects.h>

namespace ProjectExplorer { class BuildConfiguration; }

namespace QtSupport {

class QTSUPPORT_EXPORT QmlDebuggingAspect : public Utils::TriStateAspect
{
    Q_OBJECT

public:
    explicit QmlDebuggingAspect(ProjectExplorer::BuildConfiguration *buildConfig);

    void addToLayout(Utils::LayoutBuilder &builder) override;

private:
    const ProjectExplorer::BuildConfiguration *m_buildConfig = nullptr;
};

class QTSUPPORT_EXPORT QtQuickCompilerAspect : public Utils::TriStateAspect
{
    Q_OBJECT

public:
    QtQuickCompilerAspect(ProjectExplorer::BuildConfiguration *buildConfig);

private:
    void addToLayout(Utils::LayoutBuilder &builder) override;

    const ProjectExplorer::BuildConfiguration *m_buildConfig = nullptr;
};

} // namespace QtSupport
