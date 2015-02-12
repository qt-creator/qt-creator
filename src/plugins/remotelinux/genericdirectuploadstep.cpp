/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "genericdirectuploadstep.h"

#include "genericdirectuploadservice.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/target.h>

#include <QCheckBox>
#include <QVBoxLayout>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
const char IncrementalKey[] = "RemoteLinux.GenericDirectUploadStep.Incremental";
const char IgnoreMissingFilesKey[] = "RemoteLinux.GenericDirectUploadStep.IgnoreMissingFiles";

class ConfigWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    ConfigWidget(GenericDirectUploadStep *step) : SimpleBuildStepConfigWidget(step)
    {
        m_incrementalCheckBox.setText(tr("Incremental deployment"));
        m_ignoreMissingFilesCheckBox.setText(tr("Ignore missing files"));
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        mainLayout->addWidget(&m_incrementalCheckBox);
        mainLayout->addWidget(&m_ignoreMissingFilesCheckBox);
        m_incrementalCheckBox.setChecked(step->incrementalDeployment());
        m_ignoreMissingFilesCheckBox.setChecked(step->ignoreMissingFiles());
        connect(&m_incrementalCheckBox, SIGNAL(toggled(bool)),
            SLOT(handleIncrementalChanged(bool)));
        connect(&m_ignoreMissingFilesCheckBox, SIGNAL(toggled(bool)),
            SLOT(handleIgnoreMissingFilesChanged(bool)));
    }

    bool showWidget() const { return true; }

private:
    Q_SLOT void handleIncrementalChanged(bool incremental) {
        GenericDirectUploadStep *step = qobject_cast<GenericDirectUploadStep *>(this->step());
        step->setIncrementalDeployment(incremental);
    }

    Q_SLOT void handleIgnoreMissingFilesChanged(bool ignoreMissingFiles) {
        GenericDirectUploadStep *step = qobject_cast<GenericDirectUploadStep *>(this->step());
        step->setIgnoreMissingFiles(ignoreMissingFiles);
    }

    QCheckBox m_incrementalCheckBox;
    QCheckBox m_ignoreMissingFilesCheckBox;
};

} // anonymous namespace

class GenericDirectUploadStepPrivate
{
public:
    GenericDirectUploadStepPrivate() : incremental(true), ignoreMissingFiles(false) {}

    GenericDirectUploadService deployService;
    bool incremental;
    bool ignoreMissingFiles;
};

} // namespace Internal

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl, Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    ctor();
}

GenericDirectUploadStep::GenericDirectUploadStep(BuildStepList *bsl, GenericDirectUploadStep *other)
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
    deployService()->setIgnoreMissingFiles(ignoreMissingFiles());
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
    setIgnoreMissingFiles(map.value(QLatin1String(Internal::IgnoreMissingFilesKey), false).toBool());
    return true;
}

QVariantMap GenericDirectUploadStep::toMap() const
{
    QVariantMap map = AbstractRemoteLinuxDeployStep::toMap();
    map.insert(QLatin1String(Internal::IncrementalKey), incrementalDeployment());
    map.insert(QLatin1String(Internal::IgnoreMissingFilesKey), ignoreMissingFiles());
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

void GenericDirectUploadStep::setIgnoreMissingFiles(bool ignoreMissingFiles)
{
    d->ignoreMissingFiles = ignoreMissingFiles;
}

bool GenericDirectUploadStep::ignoreMissingFiles() const
{
    return d->ignoreMissingFiles;
}

Core::Id GenericDirectUploadStep::stepId()
{
    return "RemoteLinux.DirectUploadStep";
}

QString GenericDirectUploadStep::displayName()
{
    return tr("Upload files via SFTP");
}

} //namespace RemoteLinux

#include "genericdirectuploadstep.moc"
