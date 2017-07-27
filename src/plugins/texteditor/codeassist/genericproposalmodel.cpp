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

#include "genericproposalmodel.h"

#include "assistproposalitem.h"
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <QDebug>
#include <QRegularExpression>
#include <QtAlgorithms>
#include <QHash>

#include <algorithm>

using namespace TextEditor;

uint qHash(const AssistProposalItem &item)
{
    return qHash(item.text());
}

namespace {

constexpr int kMaxSort = 1000;
constexpr int kMaxPrefixFilter = 100;

struct ContentLessThan
{
    ContentLessThan(const QString &prefix)
        : m_prefix(prefix)
    {}

    bool operator()(const AssistProposalItemInterface *a, const AssistProposalItemInterface *b)
    {
        // The order is case-insensitive in principle, but case-sensitive when this
        // would otherwise mean equality
        const QString &lowera = a->text().toLower();
        const QString &lowerb = b->text().toLower();
        const QString &lowerprefix = m_prefix.toLower();

        // All continuations should go before all fuzzy matches
        if (int diff = lowera.startsWith(lowerprefix) - lowerb.startsWith(lowerprefix))
            return diff > 0;
        if (int diff = a->text().startsWith(m_prefix) - b->text().startsWith(m_prefix))
            return diff > 0;

        // If order is different, show higher ones first.
        if (a->order() != b->order())
            return a->order() > b->order();

        if (lowera == lowerb)
            return lessThan(a->text(), b->text());
        else
            return lessThan(lowera, lowerb);
    }

    bool lessThan(const QString &a, const QString &b)
    {
        QString::const_iterator pa = a.begin();
        QString::const_iterator pb = b.begin();

        CharLessThan charLessThan;
        enum { Letter, SmallerNumber, BiggerNumber } state = Letter;
        for (; pa != a.end() && pb != b.end(); ++pa, ++pb) {
            if (*pa == *pb)
                continue;
            if (state != Letter) {
                if (!pa->isDigit() || !pb->isDigit())
                    break;
            } else if (pa->isDigit() && pb->isDigit()) {
                if (charLessThan(*pa, *pb))
                    state = SmallerNumber;
                else
                    state = BiggerNumber;
            } else {
                return charLessThan(*pa, *pb);
            }
        }

        if (state == Letter)
            return pa == a.end() && pb != b.end();
        if (pa != a.end() && pa->isDigit())
            return false;                   //more digits
        if (pb != b.end() && pb->isDigit())
            return true;                    //fewer digits
        return state == SmallerNumber;      //same length, compare first different digit in the sequence
    }

    struct CharLessThan
    {
        bool operator()(const QChar &a, const QChar &b)
        {
            if (a == QLatin1Char('_'))
                return false;
            else if (b == QLatin1Char('_'))
                return true;
            else
                return a < b;
        }
    };

private:
    QString m_prefix;
};

} // Anonymous

GenericProposalModel::GenericProposalModel()
{}

GenericProposalModel::~GenericProposalModel()
{
    qDeleteAll(m_originalItems);
}

void GenericProposalModel::loadContent(const QList<AssistProposalItemInterface *> &items)
{
    m_originalItems = items;
    m_currentItems = items;
    m_duplicatesRemoved = false;
    for (int i = 0; i < m_originalItems.size(); ++i)
        m_idByText.insert(m_originalItems.at(i)->text(), i);
}

bool GenericProposalModel::hasItemsToPropose(const QString &prefix, AssistReason reason) const
{
    return size() != 0 && (keepPerfectMatch(reason) || !isPerfectMatch(prefix));
}

static QString cleanText(const QString &original)
{
    QString clean = original;
    int ignore = 0;
    for (int i = clean.length() - 1; i >= 0; --i, ++ignore) {
        const QChar &c = clean.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_')
                || c.isHighSurrogate() || c.isLowSurrogate()) {
            break;
        }
    }
    if (ignore)
        clean.chop(ignore);
    return clean;
}

bool GenericProposalModel::isPerfectMatch(const QString &prefix) const
{
    if (prefix.isEmpty())
        return false;

    for (int i = 0; i < size(); ++i) {
        const QString &current = cleanText(text(i));
        if (!current.isEmpty()) {
            CaseSensitivity cs = TextEditorSettings::completionSettings().m_caseSensitivity;
            if (cs == TextEditor::CaseSensitive) {
                if (prefix == current)
                    return true;
            } else if (cs == TextEditor::CaseInsensitive) {
                if (prefix.compare(current, Qt::CaseInsensitive) == 0)
                    return true;
            } else if (cs == TextEditor::FirstLetterCaseSensitive) {
                if (prefix.at(0) == current.at(0)
                        && prefix.midRef(1).compare(current.midRef(1), Qt::CaseInsensitive) == 0)
                    return true;
            }
        }
    }
    return false;
}

bool GenericProposalModel::isPrefiltered(const QString &prefix) const
{
    return !m_prefilterPrefix.isEmpty() && prefix == m_prefilterPrefix;
}

void GenericProposalModel::setPrefilterPrefix(const QString &prefix)
{
    m_prefilterPrefix = prefix;
}

void GenericProposalModel::reset()
{
    m_prefilterPrefix.clear();
    m_currentItems = m_originalItems;
}

int GenericProposalModel::size() const
{
    return m_currentItems.size();
}

QString GenericProposalModel::text(int index) const
{
    return m_currentItems.at(index)->text();
}

QIcon GenericProposalModel::icon(int index) const
{
    return m_currentItems.at(index)->icon();
}

QString GenericProposalModel::detail(int index) const
{
    return m_currentItems.at(index)->detail();
}

void GenericProposalModel::removeDuplicates()
{
    if (m_duplicatesRemoved)
        return;

    QHash<QString, quint64> unique;
    auto it = m_originalItems.begin();
    while (it != m_originalItems.end()) {
        const AssistProposalItemInterface *item = *it;
        if (unique.contains(item->text())
                && unique.value(item->text()) == item->hash()) {
            delete *it;
            it = m_originalItems.erase(it);
        } else {
            unique.insert(item->text(), item->hash());
            ++it;
        }
    }

    m_duplicatesRemoved = true;
}

void GenericProposalModel::filter(const QString &prefix)
{
    if (prefix.isEmpty())
        return;

    const CamelHumpMatcher::CaseSensitivity caseSensitivity =
        convertCaseSensitivity(TextEditorSettings::completionSettings().m_caseSensitivity);
    const QRegularExpression regExp = CamelHumpMatcher::createCamelHumpRegExp(prefix, caseSensitivity);

    m_currentItems.clear();
    const QString lowerPrefix = prefix.toLower();
    foreach (const auto &item, m_originalItems) {
        const QString &text = item->text();
        if (regExp.match(text).hasMatch()) {
            m_currentItems.append(item);
            if (text.startsWith(prefix)) {
                // Direct match
                item->setPrefixMatch(text.length() == prefix.length()
                                     ? AssistProposalItemInterface::PrefixMatch::Full
                                     : AssistProposalItemInterface::PrefixMatch::Exact);
                continue;
            }

            if (text.startsWith(lowerPrefix, Qt::CaseInsensitive))
                item->setPrefixMatch(AssistProposalItemInterface::PrefixMatch::Lower);
        }
    }
}

CamelHumpMatcher::CaseSensitivity
    GenericProposalModel::convertCaseSensitivity(TextEditor::CaseSensitivity textEditorCaseSensitivity)
{
    switch (textEditorCaseSensitivity) {
    case TextEditor::CaseSensitive:
        return CamelHumpMatcher::CaseSensitivity::CaseSensitive;
    case TextEditor::FirstLetterCaseSensitive:
        return CamelHumpMatcher::CaseSensitivity::FirstLetterCaseSensitive;
    default:
        return CamelHumpMatcher::CaseSensitivity::CaseInsensitive;
    }
}

bool GenericProposalModel::isSortable(const QString &prefix) const
{
    Q_UNUSED(prefix);

    if (m_currentItems.size() < kMaxSort)
        return true;
    return false;
}

void GenericProposalModel::sort(const QString &prefix)
{
    std::stable_sort(m_currentItems.begin(), m_currentItems.end(), ContentLessThan(prefix));
}

int GenericProposalModel::persistentId(int index) const
{
    return m_idByText.value(m_currentItems.at(index)->text());
}

bool GenericProposalModel::containsDuplicates() const
{
    return true;
}

bool GenericProposalModel::supportsPrefixExpansion() const
{
    return true;
}

bool GenericProposalModel::keepPerfectMatch(AssistReason reason) const
{
    return reason != IdleEditor;
}

QString GenericProposalModel::proposalPrefix() const
{
    if (m_currentItems.size() >= kMaxPrefixFilter || m_currentItems.isEmpty())
        return QString();

    // Compute common prefix
    QString commonPrefix = m_currentItems.first()->text();
    for (int i = 1, ei = m_currentItems.size(); i < ei; ++i) {
        QString nextItem = m_currentItems.at(i)->text();
        const int length = qMin(commonPrefix.length(), nextItem.length());
        commonPrefix.truncate(length);
        nextItem.truncate(length);

        while (commonPrefix != nextItem) {
            commonPrefix.chop(1);
            nextItem.chop(1);
        }

        if (commonPrefix.isEmpty()) // there is no common prefix, so return.
            return commonPrefix;
    }

    return commonPrefix;
}

AssistProposalItemInterface *GenericProposalModel::proposalItem(int index) const
{
    return m_currentItems.at(index);
}
