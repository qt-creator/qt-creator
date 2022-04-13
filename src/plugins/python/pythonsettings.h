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

#include <projectexplorer/runconfigurationaspects.h>

#include <utils/fileutils.h>
#include <utils/optional.h>

#include <QUuid>

namespace Python {
namespace Internal {

class PythonSettings : public QObject
{
    Q_OBJECT
public:
    static void init();

    using Interpreter = ProjectExplorer::Interpreter;

    static QList<Interpreter> interpreters();
    static Interpreter defaultInterpreter();
    static Interpreter interpreter(const QString &interpreterId);
    static void setInterpreter(const QList<Interpreter> &interpreters, const QString &defaultId);
    static void addInterpreter(const Interpreter &interpreter, bool isDefault = false);
    static PythonSettings *instance();

    static QList<Interpreter> detectPythonVenvs(const Utils::FilePath &path);

signals:
    void interpretersChanged(const QList<Interpreter> &interpreters, const QString &defaultId);

private:
    PythonSettings();

    static void saveSettings();
};

} // namespace Internal
} // namespace PythonEditor
