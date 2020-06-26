/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmake_global.h"

#include "cmaketool.h"

#include <utils/fileutils.h>
#include <utils/id.h>

#include <QObject>

#include <memory>

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeToolManager : public QObject
{
    Q_OBJECT
public:
    CMakeToolManager();
    ~CMakeToolManager();

    static CMakeToolManager *instance();

    static QList<CMakeTool *> cmakeTools();

    static bool registerCMakeTool(std::unique_ptr<CMakeTool> &&tool);
    static void deregisterCMakeTool(const Utils::Id &id);

    static CMakeTool *defaultCMakeTool();
    static void setDefaultCMakeTool(const Utils::Id &id);
    static CMakeTool *findByCommand(const Utils::FilePath &command);
    static CMakeTool *findById(const Utils::Id &id);

    static void notifyAboutUpdate(CMakeTool *);
    static void restoreCMakeTools();

    static void updateDocumentation();

signals:
    void cmakeAdded (const Utils::Id &id);
    void cmakeRemoved (const Utils::Id &id);
    void cmakeUpdated (const Utils::Id &id);
    void cmakeToolsChanged ();
    void cmakeToolsLoaded ();
    void defaultCMakeChanged ();

private:
    static void saveCMakeTools();
    static void ensureDefaultCMakeToolIsValid();

    static CMakeToolManager *m_instance;
};

} // namespace CMakeProjectManager
