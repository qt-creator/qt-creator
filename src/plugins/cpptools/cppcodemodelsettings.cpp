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

#include "cppcodemodelsettings.h"
#include "cppmodelmanagersupport.h"
#include "cpptoolsconstants.h"

using namespace CppTools;

static QLatin1String cppHeaderMimeType(Constants::CPP_HEADER_MIMETYPE);
static QLatin1String cHeaderMimeType(Constants::C_HEADER_MIMETYPE);

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    QVariant supporters = s->value(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_SUPPORTERS_KEY));

    foreach (const QString &mimeType, supportedMimeTypes())
        setIdForMimeType(supporters, mimeType);

    QVariant v = s->value(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE), PchUse_None);
    setPCHUsage(static_cast<PCHUsage>(v.toInt()));
    s->endGroup();

    emit changed();
}

void CppCodeModelSettings::toSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    QHash<QString, QVariant> var;
    foreach (const QString &mimeType, m_modelManagerSupportByMimeType.keys())
        var[mimeType] = m_modelManagerSupportByMimeType[mimeType];
    s->setValue(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_SUPPORTERS_KEY), QVariant(var));
    s->setValue(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE), pchUsage());
    s->endGroup();

    emit changed();
}

QStringList CppCodeModelSettings::supportedMimeTypes()
{
    return QStringList({
        QLatin1String(Constants::C_SOURCE_MIMETYPE),
        QLatin1String(Constants::CPP_SOURCE_MIMETYPE),
        QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE),
        QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE),
        QLatin1String(Constants::CPP_HEADER_MIMETYPE)
    });
}

void CppCodeModelSettings::emitChanged()
{
    emit changed();
}

void CppCodeModelSettings::setModelManagerSupportProviders(
        const QList<ModelManagerSupportProvider *> &providers)
{
    m_modelManagerSupportsByName.clear();
    foreach (ModelManagerSupportProvider *provider, providers)
        m_modelManagerSupportsByName[provider->displayName()] = provider->id();
}

QString CppCodeModelSettings::modelManagerSupportIdForMimeType(const QString &mimeType) const
{
    if (mimeType == cHeaderMimeType)
        return m_modelManagerSupportByMimeType.value(cppHeaderMimeType);
    else
        return m_modelManagerSupportByMimeType.value(mimeType);
}

void CppCodeModelSettings::setModelManagerSupportIdForMimeType(const QString &mimeType,
                                                               const QString &id)
{
    QString theMimeType = mimeType;
    if (theMimeType == cHeaderMimeType)
        theMimeType = cppHeaderMimeType;

    m_modelManagerSupportByMimeType.insert(theMimeType, id);
}

void CppCodeModelSettings::setIdForMimeType(const QVariant &var, const QString &mimeType)
{
    QHash<QString, QVariant> mimeToId = var.toHash();
    m_modelManagerSupportByMimeType[mimeType] = mimeToId.value(mimeType, defaultId()).toString();
}
