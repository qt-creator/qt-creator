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

#include "projectexplorer_export.h"
#include "ioutputparser.h"
#include "devicesupport/idevice.h"

#include <QRegExp>
#include <QStringList>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT XcodebuildParser : public IOutputParser
{
    Q_OBJECT
public:
    enum XcodebuildStatus {
        InXcodebuild,
        OutsideXcodebuild,
        UnknownXcodebuildState
    };

    XcodebuildParser();

    void stdOutput(const QString &line) override;
    void stdError(const QString &line) override;
    bool hasFatalErrors() const override;

private:
    int m_fatalErrorCount = 0;
    QRegExp m_failureRe;
    QRegExp m_successRe;
    QRegExp m_buildRe;
    XcodebuildStatus m_xcodeBuildParserState = OutsideXcodebuild;
    QString m_lastTarget;
    QString m_lastProject;

#if defined WITH_TESTS
    friend class XcodebuildParserTester;
    friend class ProjectExplorerPlugin;
#endif
};

#if defined WITH_TESTS
class XcodebuildParserTester : public QObject
{
    Q_OBJECT
public:
    explicit XcodebuildParserTester(XcodebuildParser *parser, QObject *parent = nullptr);

    XcodebuildParser *parser;
    XcodebuildParser::XcodebuildStatus expectedFinalState;

public slots:
    void onAboutToDeleteParser();
};
#endif

} // namespace ProjectExplorer
