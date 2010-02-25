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

#include "s60createpackagestep.h"

#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

using namespace Qt4ProjectManager::Internal;

namespace {
    const char * const SIGN_BS_ID = "Qt4ProjectManager.S60SignBuildStep";
    const char * const SIGNMODE_KEY("Qt4ProjectManager.S60CreatePackageStep.SignMode");
    const char * const CERTIFICATE_KEY("Qt4ProjectManager.S60CreatePackageStep.Certificate");
    const char * const KEYFILE_KEY("Qt4ProjectManager.S60CreatePackageStep.Keyfile");
}

// #pragma mark -- S60SignBuildStep

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc) :
    MakeStep(bc, QLatin1String(SIGN_BS_ID)),
    m_signingMode(SignSelf)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc, S60CreatePackageStep *bs) :
    MakeStep(bc, bs),
    m_signingMode(bs->m_signingMode),
    m_customSignaturePath(bs->m_customSignaturePath),
    m_customKeyPath(bs->m_customKeyPath)
{
    ctor_package();
}

S60CreatePackageStep::S60CreatePackageStep(ProjectExplorer::BuildConfiguration *bc, const QString &id) :
    MakeStep(bc, id),
    m_signingMode(SignSelf)
{
    ctor_package();
}

void S60CreatePackageStep::ctor_package()
{
    setDisplayName(tr("Create sis Package", "Create sis package build step name"));
}

S60CreatePackageStep::~S60CreatePackageStep()
{
}

QVariantMap S60CreatePackageStep::toMap() const
{
    QVariantMap map(MakeStep::toMap());
    map.insert(QLatin1String(SIGNMODE_KEY), (int)m_signingMode);
    map.insert(QLatin1String(CERTIFICATE_KEY), m_customSignaturePath);
    map.insert(QLatin1String(KEYFILE_KEY), m_customKeyPath);
    return map;
}

bool S60CreatePackageStep::fromMap(const QVariantMap &map)
{
    m_signingMode = (SigningMode)map.value(QLatin1String(SIGNMODE_KEY)).toInt();
    m_customSignaturePath = map.value(QLatin1String(CERTIFICATE_KEY)).toString();
    m_customKeyPath = map.value(QLatin1String(KEYFILE_KEY)).toString();
    return MakeStep::fromMap(map);
}

bool S60CreatePackageStep::init()
{
    if (!MakeStep::init())
        return false;
    Qt4BuildConfiguration *bc = qt4BuildConfiguration();
    ProjectExplorer::Environment environment = bc->environment();
    if (signingMode() == SignCustom) {
        environment.set(QLatin1String("QT_SIS_CERTIFICATE"), QDir::toNativeSeparators(customSignaturePath()));
        environment.set(QLatin1String("QT_SIS_KEY"), QDir::toNativeSeparators(customKeyPath()));
    }
    setEnvironment(environment);
    setArguments(QStringList() << "sis"); // overwrite any stuff done in make step
    return true;
}

bool S60CreatePackageStep::immutable() const
{
    return false;
}

ProjectExplorer::BuildStepConfigWidget *S60CreatePackageStep::createConfigWidget()
{
    return new S60CreatePackageStepConfigWidget(this);
}

S60CreatePackageStep::SigningMode S60CreatePackageStep::signingMode() const
{
    return m_signingMode;
}

void S60CreatePackageStep::setSigningMode(SigningMode mode)
{
    m_signingMode = mode;
}

QString S60CreatePackageStep::customSignaturePath() const
{
    return m_customSignaturePath;
}

void S60CreatePackageStep::setCustomSignaturePath(const QString &path)
{
    m_customSignaturePath = path;
}

QString S60CreatePackageStep::customKeyPath() const
{
    return m_customKeyPath;
}

void S60CreatePackageStep::setCustomKeyPath(const QString &path)
{
    m_customKeyPath = path;
}

// #pragma mark -- S60SignBuildStepFactory

S60CreatePackageStepFactory::S60CreatePackageStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

S60CreatePackageStepFactory::~S60CreatePackageStepFactory()
{
}

bool S60CreatePackageStepFactory::canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id) const
{
    if (type != ProjectExplorer::Build)
        return false;
    if (parent->target()->id() != Constants::S60_DEVICE_TARGET_ID)
        return false;
    return (id == QLatin1String(SIGN_BS_ID));
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QString &id)
{
    if (!canCreate(parent, type, id))
        return 0;
    return new S60CreatePackageStep(parent);
}

bool S60CreatePackageStepFactory::canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, type, source->id());
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, type, source))
        return 0;
    return new S60CreatePackageStep(parent, static_cast<S60CreatePackageStep *>(source));
}

bool S60CreatePackageStepFactory::canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, type, id);
}

ProjectExplorer::BuildStep *S60CreatePackageStepFactory::restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type, const QVariantMap &map)
{
    if (!canRestore(parent, type, map))
        return 0;
    S60CreatePackageStep *bs(new S60CreatePackageStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList S60CreatePackageStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::StepType type) const
{
    if (type != ProjectExplorer::Build)
        return QStringList();
    if (parent->target()->id() == Constants::S60_DEVICE_TARGET_ID)
        return QStringList() << QLatin1String(SIGN_BS_ID);
    return QStringList();
}

QString S60CreatePackageStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(SIGN_BS_ID))
        return tr("Create sis Package");
    return QString();
}

// #pragma mark -- S60SignBuildStepConfigWidget

S60CreatePackageStepConfigWidget::S60CreatePackageStepConfigWidget(S60CreatePackageStep *signStep)
    : BuildStepConfigWidget(), m_signStep(signStep)
{
    m_ui.setupUi(this);
    updateUi();
    connect(m_ui.customCertificateButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.selfSignedButton, SIGNAL(clicked()),
            this, SLOT(updateFromUi()));
    connect(m_ui.signaturePath, SIGNAL(changed(QString)),
            this, SLOT(updateFromUi()));
    connect(m_ui.keyFilePath, SIGNAL(changed(QString)),
            this, SLOT(updateFromUi()));
}

void S60CreatePackageStepConfigWidget::updateUi()
{
    bool selfSigned = m_signStep->signingMode() == S60CreatePackageStep::SignSelf;
    m_ui.selfSignedButton->setChecked(selfSigned);
    m_ui.customCertificateButton->setChecked(!selfSigned);
    m_ui.signaturePath->setEnabled(!selfSigned);
    m_ui.keyFilePath->setEnabled(!selfSigned);
    m_ui.signaturePath->setPath(m_signStep->customSignaturePath());
    m_ui.keyFilePath->setPath(m_signStep->customKeyPath());
    emit updateSummary();
}

void S60CreatePackageStepConfigWidget::updateFromUi()
{
    bool selfSigned = m_ui.selfSignedButton->isChecked();
    m_signStep->setSigningMode(selfSigned ? S60CreatePackageStep::SignSelf
        : S60CreatePackageStep::SignCustom);
    m_signStep->setCustomSignaturePath(m_ui.signaturePath->path());
    m_signStep->setCustomKeyPath(m_ui.keyFilePath->path());
    updateUi();
}

QString S60CreatePackageStepConfigWidget::summaryText() const
{
    QString text;
    if (m_signStep->signingMode() == S60CreatePackageStep::SignSelf) {
        text = tr("self-signed");
    } else {
        text = tr("signed with certificate %1 and key file %2")
               .arg(m_signStep->customSignaturePath())
               .arg(m_signStep->customKeyPath());
    }
    return tr("<b>Create sis Package:</b> %1").arg(text);
}

QString S60CreatePackageStepConfigWidget::displayName() const
{
    return m_signStep->displayName();
}

void S60CreatePackageStepConfigWidget::init()
{
}
