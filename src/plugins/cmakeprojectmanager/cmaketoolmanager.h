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
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <functional>

#include <QObject>

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeToolManager : public QObject
{
    Q_OBJECT
public:
    typedef std::function<QList<CMakeTool *> ()> AutodetectionHelper;

    CMakeToolManager(QObject *parent);
    ~CMakeToolManager() override;

    static CMakeToolManager *instance();

    static QList<CMakeTool *> cmakeTools();

    static Core::Id registerOrFindCMakeTool(const Utils::FileName &command);
    static bool registerCMakeTool(CMakeTool *tool);
    static void deregisterCMakeTool(const Core::Id &id);

    static CMakeTool *defaultCMakeTool();
    static void setDefaultCMakeTool(const Core::Id &id);
    static CMakeTool *findByCommand(const Utils::FileName &command);
    static CMakeTool *findById(const Core::Id &id);
    static void registerAutodetectionHelper(AutodetectionHelper helper);

    static void notifyAboutUpdate(CMakeTool *);
    static void restoreCMakeTools();

signals:
    void cmakeAdded (const Core::Id &id);
    void cmakeRemoved (const Core::Id &id);
    void cmakeUpdated (const Core::Id &id);
    void cmakeToolsChanged ();
    void cmakeToolsLoaded ();
    void defaultCMakeChanged ();

private:
    static void saveCMakeTools();

    static CMakeToolManager *m_instance;
};

} // namespace CMakeProjectManager
