/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef ANALYZERPLUGIN_H
#define ANALYZERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Analyzer {
namespace Internal {

class AnalyzerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "AnalyzerBase.json")

public:
    static AnalyzerPlugin *instance();

    AnalyzerPlugin();
    virtual ~AnalyzerPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized() {}

    ShutdownFlag aboutToShutdown();
};

} // namespace Internal
} // namespace Analyzer

#endif // ANALYZERPLUGIN_H
