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

#include "commentssettings.h"

#include <QSettings>

using namespace TextEditor;

namespace {

const char kDocumentationCommentsGroup[] = "CppToolsDocumentationComments";
const char kEnableDoxygenBlocks[] = "EnableDoxygenBlocks";
const char kGenerateBrief[] = "GenerateBrief";
const char kAddLeadingAsterisks[] = "AddLeadingAsterisks";

}

CommentsSettings::CommentsSettings()
    : m_enableDoxygen(true)
    , m_generateBrief(true)
    , m_leadingAsterisks(true)
{}

void CommentsSettings::toSettings(QSettings *s) const
{
    s->beginGroup(kDocumentationCommentsGroup);
    s->setValue(kEnableDoxygenBlocks, m_enableDoxygen);
    s->setValue(kGenerateBrief, m_generateBrief);
    s->setValue(kAddLeadingAsterisks, m_leadingAsterisks);
    s->endGroup();
}

void CommentsSettings::fromSettings(QSettings *s)
{
    s->beginGroup(kDocumentationCommentsGroup);
    m_enableDoxygen = s->value(kEnableDoxygenBlocks, true).toBool();
    m_generateBrief = m_enableDoxygen && s->value(kGenerateBrief, true).toBool();
    m_leadingAsterisks = s->value(kAddLeadingAsterisks, true).toBool();
    s->endGroup();
}

bool CommentsSettings::equals(const CommentsSettings &other) const
{
    return m_enableDoxygen == other.m_enableDoxygen
            && m_generateBrief == other.m_generateBrief
            && m_leadingAsterisks == other.m_leadingAsterisks;
}
