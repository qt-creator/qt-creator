/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_NO_CAST_FROM_ASCII

#include "qmimemagicrulematcher_p.h"

#include "qmimetype_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QMimeMagicRuleMatcher
    \inmodule QtCore

    \brief The QMimeMagicRuleMatcher class checks a number of rules based on operator "or".

    It is used for rules parsed from XML files.

    \sa QMimeType, QMimeDatabase, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa QMimeTypeParserBase, MimeTypeParser
*/

QMimeMagicRuleMatcher::QMimeMagicRuleMatcher(const QString &mime, unsigned thePriority) :
    m_list(),
    m_priority(thePriority),
    m_mimetype(mime)
{
}

bool QMimeMagicRuleMatcher::operator==(const QMimeMagicRuleMatcher &other) const
{
    return m_list == other.m_list &&
           m_priority == other.m_priority;
}

void QMimeMagicRuleMatcher::addRule(const QMimeMagicRule &rule)
{
    m_list.append(rule);
}

void QMimeMagicRuleMatcher::addRules(const QList<QMimeMagicRule> &rules)
{
    m_list.append(rules);
}

QList<QMimeMagicRule> QMimeMagicRuleMatcher::magicRules() const
{
    return m_list;
}

// Check for a match on contents of a file
bool QMimeMagicRuleMatcher::matches(const QByteArray &data) const
{
    for (const QMimeMagicRule &magicRule : m_list) {
        if (magicRule.matches(data))
            return true;
    }

    return false;
}

// Return a priority value from 1..100
unsigned QMimeMagicRuleMatcher::priority() const
{
    return m_priority;
}

QT_END_NAMESPACE
