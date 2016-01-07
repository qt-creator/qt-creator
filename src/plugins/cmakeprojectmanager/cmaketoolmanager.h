/****************************************************************************
**
** Copyright (C) 2015 Canonical Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKETOOLMANAGER_H
#define CMAKETOOLMANAGER_H

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
    static void setPreferNinja(bool set);
    static bool preferNinja();

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

#endif // CMAKETOOLMANAGER_H
