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

class QdbMakeDefaultAppStepPrivate
{
public:
    QdbMakeDefaultAppService deployService;
    bool makeDefault;
};

class QdbConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QdbConfigWidget(QdbMakeDefaultAppStep *step)
        : BuildStepConfigWidget(step)
    {
        QVBoxLayout * const mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);

        m_makeDefaultBtn.setText(tr("Set this application to start by default"));
        m_resetDefaultBtn.setText(tr("Reset default application"));

        if (step->makeDefault())
            m_makeDefaultBtn.setChecked(true);
        else
            m_resetDefaultBtn.setChecked(true);

        mainLayout->addWidget(&m_makeDefaultBtn);
        mainLayout->addWidget(&m_resetDefaultBtn);

        connect(&m_makeDefaultBtn, &QRadioButton::clicked,
                this, &QdbConfigWidget::handleMakeDefault);
        connect(&m_resetDefaultBtn, &QRadioButton::clicked,
                this, &QdbConfigWidget::handleResetDefault);
    }

private:
    Q_SLOT void handleMakeDefault() {
        QdbMakeDefaultAppStep *step =
                qobject_cast<QdbMakeDefaultAppStep *>(this->step());
        step->setMakeDefault(true);
    }

    Q_SLOT void handleResetDefault() {
        QdbMakeDefaultAppStep *step =
                qobject_cast<QdbMakeDefaultAppStep *>(this->step());
        step->setMakeDefault(false);
    }

    QRadioButton m_makeDefaultBtn;
    QRadioButton m_resetDefaultBtn;
};

QdbMakeDefaultAppStep::QdbMakeDefaultAppStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractRemoteLinuxDeployStep(bsl, stepId())
{
    d = new QdbMakeDefaultAppStepPrivate;
    setDefaultDisplayName(stepDisplayName());

}

QdbMakeDefaultAppStep::~QdbMakeDefaultAppStep()
{
    delete d;
}

Core::Id QdbMakeDefaultAppStep::stepId()
{
    return "Qdb.MakeDefaultAppStep";
}

RemoteLinux::CheckResult QdbMakeDefaultAppStep::initInternal()
{
    d->deployService.setMakeDefault(d->makeDefault);
    return deployService()->isDeploymentPossible();
}

RemoteLinux::AbstractRemoteLinuxDeployService *QdbMakeDefaultAppStep::deployService() const
{
    return &d->deployService;
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
    d->makeDefault = makeDefault;
}

bool QdbMakeDefaultAppStep::makeDefault() const
{
    return d->makeDefault;
}

static QString makeDefaultKey()
{
    return QLatin1String("QdbMakeDefaultDeployStep.MakeDefault");
}

bool QdbMakeDefaultAppStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    d->makeDefault = map.value(makeDefaultKey()).toBool();
    return true;
}

QVariantMap QdbMakeDefaultAppStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(makeDefaultKey(), d->makeDefault);
    return map;
}
} // namespace Internal
} // namespace Qdb

#include "qdbmakedefaultappstep.moc"
