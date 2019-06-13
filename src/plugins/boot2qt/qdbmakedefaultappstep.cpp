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

#include "qdbmakedefaultappstep.h"

#include "qdbmakedefaultappservice.h"

#include <QRadioButton>
#include <QVBoxLayout>

namespace Qdb {
namespace Internal {

class QdbConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    QdbConfigWidget(QdbMakeDefaultAppStep *step)
        : BuildStepConfigWidget(step)
    {
        QVBoxLayout * const mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);

        m_makeDefaultBtn.setText(
                    QdbMakeDefaultAppStep::tr("Set this application to start by default"));
        m_resetDefaultBtn.setText(
                    QdbMakeDefaultAppStep::tr("Reset default application"));

        if (step->makeDefault())
            m_makeDefaultBtn.setChecked(true);
        else
            m_resetDefaultBtn.setChecked(true);

        mainLayout->addWidget(&m_makeDefaultBtn);
        mainLayout->addWidget(&m_resetDefaultBtn);

        connect(&m_makeDefaultBtn, &QRadioButton::clicked, this, [step] {
            step->setMakeDefault(true);
        });
        connect(&m_resetDefaultBtn, &QRadioButton::clicked, this, [step] {
            step->setMakeDefault(false);
        });
    }

private:
    QRadioButton m_makeDefaultBtn;
    QRadioButton m_resetDefaultBtn;
};

QdbMakeDefaultAppStep::QdbMakeDefaultAppStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    setDefaultDisplayName(stepDisplayName());

    auto service = createDeployService<QdbMakeDefaultAppService>();

    setInternalInitializer([this, service] {
        service->setMakeDefault(m_makeDefault);
        return service->isDeploymentPossible();
    });
}

Core::Id QdbMakeDefaultAppStep::stepId()
{
    return "Qdb.MakeDefaultAppStep";
}

ProjectExplorer::BuildStepConfigWidget *QdbMakeDefaultAppStep::createConfigWidget()
{
    return new QdbConfigWidget(this);
}

QString QdbMakeDefaultAppStep::stepDisplayName()
{
    return QStringLiteral("Change default application");
}

void QdbMakeDefaultAppStep::setMakeDefault(bool makeDefault)
{
    m_makeDefault = makeDefault;
}

bool QdbMakeDefaultAppStep::makeDefault() const
{
    return m_makeDefault;
}

static QString makeDefaultKey()
{
    return QLatin1String("QdbMakeDefaultDeployStep.MakeDefault");
}

bool QdbMakeDefaultAppStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    m_makeDefault = map.value(makeDefaultKey()).toBool();
    return true;
}

QVariantMap QdbMakeDefaultAppStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(makeDefaultKey(), m_makeDefault);
    return map;
}

} // namespace Internal
} // namespace Qdb
