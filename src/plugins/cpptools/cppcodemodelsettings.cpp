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

#include "cppcodemodelsettings.h"
#include "cpptoolsconstants.h"

#include <QSettings>

using namespace CppTools;

static QLatin1String cppHeaderMimeType(Constants::CPP_HEADER_MIMETYPE);
static QLatin1String cHeaderMimeType(Constants::C_HEADER_MIMETYPE);
static QLatin1String clangExtraOptionsKey(Constants::CPPTOOLS_EXTRA_CLANG_OPTIONS);

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));

    setExtraClangOptions(s->value(clangExtraOptionsKey, defaultExtraClangOptions()).toStringList());

    QVariant v = s->value(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE), PchUse_None);
    setPCHUsage(static_cast<PCHUsage>(v.toInt()));
    s->endGroup();

    emit changed();
}

void CppCodeModelSettings::toSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));

    s->setValue(clangExtraOptionsKey, extraClangOptions());
    s->setValue(QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE), pchUsage());

    s->endGroup();

    emit changed();
}

QStringList CppCodeModelSettings::defaultExtraClangOptions()
{
    return {
        QStringLiteral("-Weverything"),
        QStringLiteral("-Wno-c++98-compat"),
        QStringLiteral("-Wno-c++98-compat-pedantic"),
        QStringLiteral("-Wno-unused-macros"),
        QStringLiteral("-Wno-newline-eof"),
        QStringLiteral("-Wno-exit-time-destructors"),
        QStringLiteral("-Wno-global-constructors"),
        QStringLiteral("-Wno-gnu-zero-variadic-macro-arguments"),
        QStringLiteral("-Wno-documentation"),
        QStringLiteral("-Wno-shadow"),
        QStringLiteral("-Wno-missing-prototypes"), // Not optimal for C projects.
    };
}

QStringList CppCodeModelSettings::extraClangOptions() const
{
    return m_extraClangOptions;
}

void CppCodeModelSettings::setExtraClangOptions(const QStringList &extraClangOptions)
{
    m_extraClangOptions = extraClangOptions;
}

CppCodeModelSettings::PCHUsage CppCodeModelSettings::pchUsage() const
{
    return m_pchUsage;
}

void CppCodeModelSettings::setPCHUsage(CppCodeModelSettings::PCHUsage pchUsage)
{
    m_pchUsage = pchUsage;
}

void CppCodeModelSettings::emitChanged()
{
    emit changed();
}
