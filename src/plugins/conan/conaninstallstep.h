/****************************************************************************
**
** Copyright (C) 2018 Jochen Seemann
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

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/task.h>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace ConanPackageManager {
namespace Internal {

class AutotoolsProject;
class ConanInstallStep;
class ConanInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    ConanInstallStepFactory();
};

class ConanInstallStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class ConanInstallStepFactory;

public:
    ConanInstallStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    bool init() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const;
    QStringList arguments() const;
    QString relativeSubdir() const;
    QStringList additionalArguments() const;
    QVariantMap toMap() const override;

    void setRelativeSubdir(const QString &subdir);
    void setAdditionalArguments(const QString &list);

    void setupOutputFormatter(Utils::OutputFormatter *formatter) final;

signals:
    void relativeSubdirChanged(const QString &);
    void additionalArgumentsChanged(const QString &);

private:
    bool fromMap(const QVariantMap &map) override;

    QString m_arguments;
    QString m_relativeSubdir;
    QString m_additionalArguments;
};

class ConanInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    ConanInstallStepConfigWidget(ConanInstallStep *installStep);

    QString displayName() const;
    QString summaryText() const;

private:
    void updateDetails();

    ConanInstallStep *m_installStep;
    QString m_summaryText;
    QLineEdit *m_relativeSubdir;
    QLineEdit *m_additionalArguments;
};

} // namespace Internal
} // namespace ConanPackageManager
