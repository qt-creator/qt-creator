/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlmakestep.h"
#include "qmlprojectconstants.h"
#include "qmlproject.h"

#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;

QmlMakeStep::QmlMakeStep(QmlProject *pro)
    : AbstractMakeStep(pro), m_pro(pro)
{
}

QmlMakeStep::~QmlMakeStep()
{
}

bool QmlMakeStep::init(const QString &buildConfiguration)
{
    return AbstractMakeStep::init(buildConfiguration);
}

void QmlMakeStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractMakeStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface = 0;
}

QString QmlMakeStep::name()
{
    return Constants::MAKESTEP;
}

QString QmlMakeStep::displayName()
{
    return QLatin1String("QML Make");
}

ProjectExplorer::BuildStepConfigWidget *QmlMakeStep::createConfigWidget()
{
    return new QmlMakeStepConfigWidget(this);
}

bool QmlMakeStep::immutable() const
{
    return true;
}

void QmlMakeStep::stdOut(const QString &line)
{
    AbstractMakeStep::stdOut(line);
}

QmlProject *QmlMakeStep::project() const
{
    return m_pro;
}

bool QmlMakeStep::buildsTarget(const QString &, const QString &) const
{
    return true;
}

void QmlMakeStep::setBuildTarget(const QString &, const QString &, bool)
{
}

QStringList QmlMakeStep::additionalArguments(const QString &) const
{
    return QStringList();
}

void QmlMakeStep::setAdditionalArguments(const QString &, const QStringList &)
{
}

//
// QmlMakeStepConfigWidget
//

QmlMakeStepConfigWidget::QmlMakeStepConfigWidget(QmlMakeStep *makeStep)
    : m_makeStep(makeStep)
{
}

QString QmlMakeStepConfigWidget::displayName() const
{
    return QLatin1String("QML Make");
}

void QmlMakeStepConfigWidget::init(const QString &)
{
}

QString QmlMakeStepConfigWidget::summaryText() const
{
    return tr("<b>QML Make</b>");
}

//
// QmlMakeStepFactory
//

bool QmlMakeStepFactory::canCreate(const QString &name) const
{
    return (Constants::MAKESTEP == name);
}

ProjectExplorer::BuildStep *QmlMakeStepFactory::create(ProjectExplorer::Project *project, const QString &name) const
{
    Q_ASSERT(name == Constants::MAKESTEP);
    QmlProject *pro = qobject_cast<QmlProject *>(project);
    Q_ASSERT(pro);
    return new QmlMakeStep(pro);
}

QStringList QmlMakeStepFactory::canCreateForProject(ProjectExplorer::Project *) const
{
    return QStringList();
}

QString QmlMakeStepFactory::displayNameForName(const QString &) const
{
    return QLatin1String("QML Make");
}

