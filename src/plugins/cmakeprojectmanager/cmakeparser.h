/****************************************************************************
**
** Copyright (C) 2016 Axonian LLC.
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

#ifndef CMAKEPARSER_H
#define CMAKEPARSER_H

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeParser : public ProjectExplorer::IOutputParser
{
    Q_OBJECT

public:
    explicit CMakeParser();
    void stdError(const QString &line) override;

protected:
    void doFlush() override;

private:
    ProjectExplorer::Task m_lastTask;
    QRegExp m_commonError;
    QRegExp m_nextSubError;
    bool m_skippedFirstEmptyLine = false;
    int m_lines = 0;
};

} // namespace CMakeProjectManager
} // namespace Internal

#endif // CMAKEPARSER_H
