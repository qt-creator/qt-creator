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

#ifndef PYTHONEDITOR_PLUGIN_H
#define PYTHONEDITOR_PLUGIN_H

#include <extensionsystem/iplugin.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <QSet>

namespace PythonEditor {
namespace Internal {

class PythonEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PythonEditor.json")

public:
    PythonEditorPlugin();
    virtual ~PythonEditorPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorMessage);
    virtual void extensionsInitialized() {}

    static QSet<QString> keywords();
    static QSet<QString> magics();
    static QSet<QString> builtins();

private:
    QSet<QString> m_keywords;
    QSet<QString> m_magics;
    QSet<QString> m_builtins;
};

} // namespace Internal
} // namespace PythonEditor

#endif // PYTHONEDITOR_PLUGIN_H
