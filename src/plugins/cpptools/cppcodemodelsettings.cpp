/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppcodemodelsettings.h"
#include "cppmodelmanagersupport.h"
#include "cpptoolsconstants.h"

using namespace CppTools;
using namespace CppTools::Internal;

static QLatin1String cppHeaderMimeType(Constants::CPP_HEADER_MIMETYPE);
static QLatin1String cHeaderMimeType(Constants::C_HEADER_MIMETYPE);

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    QVariant supporters = s->value(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_SUPPORTERS_KEY));
    setIdForMimeType(supporters, QLatin1String(Constants::C_SOURCE_MIMETYPE));
    setIdForMimeType(supporters, QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
    setIdForMimeType(supporters, QLatin1String(Constants::OBJECTIVE_C_SOURCE_MIMETYPE));
    setIdForMimeType(supporters, QLatin1String(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE));
    setIdForMimeType(supporters, QLatin1String(Constants::CPP_HEADER_MIMETYPE));
    QVariant v = s->value(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE), PchUse_None);
    setPCHUsage(static_cast<PCHUsage>(v.toInt()));
    s->endGroup();
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
}

void CppCodeModelSettings::setModelManagerSupports(const QList<ModelManagerSupport *> &supporters)
{
    m_availableModelManagerSupportersByName.clear();
    foreach (ModelManagerSupport *supporter, supporters)
        m_availableModelManagerSupportersByName[supporter->displayName()] = supporter->id();
}

QString CppCodeModelSettings::modelManagerSupportId(const QString &mimeType) const
{
    if (mimeType == cHeaderMimeType)
        return m_modelManagerSupportByMimeType.value(cppHeaderMimeType);
    else
        return m_modelManagerSupportByMimeType.value(mimeType);
}

void CppCodeModelSettings::setModelManagerSupportId(const QString &mimeType,
                                                    const QString &supporter)
{
    if (mimeType == cHeaderMimeType)
        m_modelManagerSupportByMimeType.insert(cppHeaderMimeType, supporter);
    else
        m_modelManagerSupportByMimeType.insert(mimeType, supporter);
}

void CppCodeModelSettings::setIdForMimeType(const QVariant &var, const QString &mimeType)
{
    QHash<QString, QVariant> mimeToId = var.toHash();
    m_modelManagerSupportByMimeType[mimeType] = mimeToId.value(mimeType, defaultId()).toString();
}
