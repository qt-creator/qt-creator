/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#include "genericdirectuploadstep.h"

#include "genericdirectuploadservice.h"
#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/target.h>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QList>
#include <QSharedPointer>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
const char IncrementalKey[] = "RemoteLinux.GenericDirectUploadStep.Incremental";

class ConfigWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    ConfigWidget(GenericDirectUploadStep *step) : SimpleBuildStepConfigWidget(step)
    {
        m_incrementalCheckBox.setText(tr("Incremental deployment"));
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        mainLayout->addWidget(&m_incrementalCheckBox);
        m_incrementalCheckBox.setChecked(step->incrementalDeployment());
        connect(&m_incrementalCheckBox, SIGNAL(toggled(bool)),
            SLOT(handleIncrementalChanged(bool)));
    }

    bool showWidget() const { return true; }

private:
    Q_SLOT void handleIncrementalChanged(bool incremental) {
        GenericDirectUploadStep *step = qobject_cast<GenericDirectUploadStep *>(this->step());
        step->setIncrementalDeployment(incremental);
    }

    QCheckBox m_incrementalCheckBox;
};

} // anonymous namespace

class GenericDirectUploadStepPrivate
{
public:
    GenericDirectUploadStepPrivate() : incremental(true) {}

    GenericDirectUploadService deployService;
    bool incremental;
};

} // namespace Internal

GenericDirectUploadStep::GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    ctor();
}

GenericDirectUploadStep::GenericDirectUploadStep(ProjectExplorer::BuildStepList *bsl, GenericDirectUploadStep *other)
    : AbstractRemoteLinuxDeployStep(bsl, other)
{
    ctor();
}

GenericDirectUploadStep::~GenericDirectUploadStep()
{
    delete d;
}

BuildStepConfigWidget *GenericDirectUploadStep::createConfigWidget()
{
    return new Internal::ConfigWidget(this);
}

bool GenericDirectUploadStep::initInternal(QString *error)
{
    deployService()->setDeployableFiles(target()->deploymentData().allFiles());
    deployService()->setIncrementalDeployment(incrementalDeployment());
    return deployService()->isDeploymentPossible(error);
}

GenericDirectUploadService *GenericDirectUploadStep::deployService() const
{
    return &d->deployService;
}

bool GenericDirectUploadStep::fromMap(const QVariantMap &map)
{
    if (!AbstractRemoteLinuxDeployStep::fromMap(map))
        return false;
    setIncrementalDeployment(map.value(QLatin1String(Internal::IncrementalKey), true).toBool());
    return true;
}

QVariantMap GenericDirectUploadStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(QLatin1String(Internal::IncrementalKey), incrementalDeployment());
    return map;
}

void GenericDirectUploadStep::ctor()
{
    setDefaultDisplayName(displayName());
    d = new Internal::GenericDirectUploadStepPrivate;
}

void GenericDirectUploadStep::setIncrementalDeployment(bool incremental)
{
    d->incremental = incremental;
}

bool GenericDirectUploadStep::incrementalDeployment() const
{
    return d->incremental;
}

Core::Id GenericDirectUploadStep::stepId()
{
    return Core::Id("RemoteLinux.DirectUploadStep");
}

QString GenericDirectUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} //namespace RemoteLinux

#include "genericdirectuploadstep.moc"
