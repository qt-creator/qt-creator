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

#include "commentssettings.h"

#include <QSettings>

using namespace CppTools;

namespace {

const char kDocumentationCommentsGroup[] = "DocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";

}

CommentsSettings::CommentsSettings()
    : m_enableDoxygen(true)
    , m_generateBrief(true)
    , m_leadingAsterisks(true)
{}

void CommentsSettings::toSettings(const QString &category, QSettings *s) const
{
    s->beginGroup(category + QLatin1String(kDocumentationCommentsGroup));
    s->setValue(QLatin1String(kEnableDoxygenBlocks), m_enableDoxygen);
    s->setValue(QLatin1String(kGenerateBrief), m_generateBrief);
    s->setValue(QLatin1String(kAddLeadingAsterisks), m_leadingAsterisks);
    s->endGroup();
}

void CommentsSettings::fromSettings(const QString &category, QSettings *s)
{
    s->beginGroup(category + QLatin1String(kDocumentationCommentsGroup));
    m_enableDoxygen = s->value(QLatin1String(kEnableDoxygenBlocks), true).toBool();
    m_generateBrief = m_enableDoxygen
            && s->value(QLatin1String(kGenerateBrief), true).toBool();
    m_leadingAsterisks = s->value(QLatin1String(kAddLeadingAsterisks), true).toBool();
    s->endGroup();
}

bool CommentsSettings::equals(const CommentsSettings &other) const
{
    return m_enableDoxygen == other.m_enableDoxygen
            && m_generateBrief == other.m_generateBrief
            && m_leadingAsterisks == other.m_leadingAsterisks;
}
