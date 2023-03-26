// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
