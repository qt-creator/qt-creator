// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericproposalmodel.h"

#include "assistproposalitem.h"
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QtAlgorithms>
#include <QHash>

#include <algorithm>
#include <utils/algorithm.h>

using namespace TextEditor;

QT_BEGIN_NAMESPACE
auto qHash(const AssistProposalItem &item)
{
    return qHash(item.text());
}
QT_END_NAMESPACE

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
            if (a == '_')
                return false;
            if (b == '_')
                return true;
            return a < b;
        }
    };

private:
    QString m_prefix;
};

} // Anonymous

GenericProposalModel::GenericProposalModel() = default;

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
        if (c.isLetterOrNumber() || c == '_'
                || c.isHighSurrogate() || c.isLowSurrogate()) {
            break;
        }
    }
    if (ignore)
        clean.chop(ignore);
    return clean;
}

static bool textStartsWith(CaseSensitivity cs, const QString &text, const QString &prefix)
{
    switch (cs) {
    case TextEditor::CaseInsensitive:
        return text.startsWith(prefix, Qt::CaseInsensitive);
    case TextEditor::CaseSensitive:
        return text.startsWith(prefix, Qt::CaseSensitive);
    case TextEditor::FirstLetterCaseSensitive:
        return prefix.at(0) == text.at(0)
               && QStringView(text).mid(1).startsWith(QStringView(prefix).mid(1), Qt::CaseInsensitive);
    }

    return false;
}

enum class PerfectMatchType {
    None,
    StartsWith,
    Full,
};

static PerfectMatchType perfectMatch(CaseSensitivity cs, const QString &text, const QString &prefix)
{
    if (textStartsWith(cs, text, prefix))
        return prefix.size() == text.size() ? PerfectMatchType::Full : PerfectMatchType::StartsWith;

    return PerfectMatchType::None;
}

bool GenericProposalModel::isPerfectMatch(const QString &prefix) const
{
    if (prefix.isEmpty())
        return false;

    const CaseSensitivity cs = TextEditorSettings::completionSettings().m_caseSensitivity;
    bool hasFullMatch = false;

    for (int i = 0; i < size(); ++i) {
        const QString &current = cleanText(text(i));
        if (current.isEmpty())
            continue;

        const PerfectMatchType match = perfectMatch(cs, current, prefix);
        if (match == PerfectMatchType::StartsWith)
            return false;

        if (match == PerfectMatchType::Full) {
            if (proposalItem(i)->isKeyword())
                return true;

            if (!proposalItem(i)->isSnippet())
                hasFullMatch = true;
        }
    }

    return hasFullMatch;
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

Qt::TextFormat GenericProposalModel::detailFormat(int index) const
{
    return m_currentItems.at(index)->detailFormat();
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

    const FuzzyMatcher::CaseSensitivity caseSensitivity =
        convertCaseSensitivity(TextEditorSettings::completionSettings().m_caseSensitivity);
    const QRegularExpression regExp = FuzzyMatcher::createRegExp(prefix, caseSensitivity);

    QElapsedTimer timer;
    timer.start();

    m_currentItems.clear();
    const QString lowerPrefix = prefix.toLower();
    const bool checkInfix = prefix.size() >= 3;
    for (const auto &item : std::as_const(m_originalItems)) {
        const QString &text = item->filterText();

        // Direct match?
        if (text.startsWith(prefix)) {
            m_currentItems.append(item);
            item->setProposalMatch(text.length() == prefix.length()
                                   ? AssistProposalItemInterface::ProposalMatch::Full
                                   : AssistProposalItemInterface::ProposalMatch::Exact);
            continue;
        }

        switch (caseSensitivity) {
        case FuzzyMatcher::CaseSensitivity::CaseInsensitive:
            if (text.startsWith(lowerPrefix, Qt::CaseInsensitive)) {
                m_currentItems.append(item);
                item->setProposalMatch(AssistProposalItemInterface::ProposalMatch::Prefix);
                continue;
            }
            if (checkInfix && text.contains(lowerPrefix, Qt::CaseInsensitive)) {
                m_currentItems.append(item);
                item->setProposalMatch(AssistProposalItemInterface::ProposalMatch::Infix);
                continue;
            }
            break;
        case FuzzyMatcher::CaseSensitivity::CaseSensitive:
            if (checkInfix && text.contains(prefix)) {
                m_currentItems.append(item);
                item->setProposalMatch(AssistProposalItemInterface::ProposalMatch::Infix);
                continue;
            }
            break;
        case FuzzyMatcher::CaseSensitivity::FirstLetterCaseSensitive:
            if (text.startsWith(prefix.at(0))
                && text.mid(1).startsWith(lowerPrefix.mid(1), Qt::CaseInsensitive)) {
                m_currentItems.append(item);
                item->setProposalMatch(AssistProposalItemInterface::ProposalMatch::Prefix);
                continue;
            }
            if (checkInfix) {
                for (auto index = text.indexOf(prefix.at(0)); index >= 0;
                     index = text.indexOf(prefix.at(0), index + 1)) {
                    if (text.mid(index + 1).startsWith(lowerPrefix.mid(1), Qt::CaseInsensitive)) {
                        m_currentItems.append(item);
                        item->setProposalMatch(AssistProposalItemInterface::ProposalMatch::Infix);
                        continue;
                    }
                }
            }
            break;
        }

        // Our fuzzy matcher can become unusably slow with certain inputs, so skip it
        // if we'd become unresponsive. See QTCREATORBUG-25419.
        if (timer.elapsed() > 100)
            continue;

        const QRegularExpressionMatch match = regExp.match(text);
        const bool hasPrefixMatch = match.capturedStart() == 0;
        const bool hasInfixMatch = checkInfix && match.hasMatch();
        if (hasPrefixMatch || hasInfixMatch)
            m_currentItems.append(item);
    }
}

FuzzyMatcher::CaseSensitivity
    GenericProposalModel::convertCaseSensitivity(TextEditor::CaseSensitivity textEditorCaseSensitivity)
{
    switch (textEditorCaseSensitivity) {
    case TextEditor::CaseSensitive:
        return FuzzyMatcher::CaseSensitivity::CaseSensitive;
    case TextEditor::FirstLetterCaseSensitive:
        return FuzzyMatcher::CaseSensitivity::FirstLetterCaseSensitive;
    default:
        return FuzzyMatcher::CaseSensitivity::CaseInsensitive;
    }
}

bool GenericProposalModel::isSortable(const QString &prefix) const
{
    Q_UNUSED(prefix)

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

int GenericProposalModel::indexOf(
    const std::function<bool(AssistProposalItemInterface *)> &predicate) const
{
    for (int index = 0, end = m_currentItems.size(); index < end; ++index) {
        if (predicate(m_currentItems.at(index)))
            return index;
    }
    return -1;
}
