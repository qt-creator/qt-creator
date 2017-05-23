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

#include <coreplugin/id.h>
#include <projectexplorer/project.h>

#include <QObject>
#include <QString>

namespace ClangCodeModel {
namespace Internal {

class ClangProjectSettings: public QObject
{
    Q_OBJECT

public:
    constexpr static const char* DelayedTemplateParsing = "-fdelayed-template-parsing";
    constexpr static const char* NoDelayedTemplateParsing = "-fno-delayed-template-parsing";
    constexpr static const char* GlobalWindowsCmdOptions = NoDelayedTemplateParsing;

    ClangProjectSettings(ProjectExplorer::Project *project);

    bool useGlobalConfig() const;
    void setUseGlobalConfig(bool useGlobalConfig);

    Core::Id warningConfigId() const;
    void setWarningConfigId(const Core::Id &warningConfigId);

    QStringList commandLineOptions() const;
    void setCommandLineOptions(const QStringList &options);

    void load();
    void store();

    static QStringList globalCommandLineOptions();

private:
    ProjectExplorer::Project *m_project;
    bool m_useGlobalConfig = true;
    Core::Id m_warningConfigId;

    QStringList m_customCommandLineOptions;
};

} // namespace Internal
} // namespace ClangCodeModel
