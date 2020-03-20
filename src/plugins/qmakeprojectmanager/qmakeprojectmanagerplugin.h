/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <extensionsystem/iplugin.h>

namespace ProjectExplorer { class Project; }

namespace QmakeProjectManager {

class QmakeProFileNode;

namespace Internal {

class QmakeProjectManagerPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmakeProjectManager.json")

public:
    ~QmakeProjectManagerPlugin() final;

#ifdef WITH_TESTS
private slots:
    void testQmakeOutputParsers_data();
    void testQmakeOutputParsers();
    void testMakefileParser_data();
    void testMakefileParser();
#endif

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;

    class QmakeProjectManagerPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace QmakeProjectManager
