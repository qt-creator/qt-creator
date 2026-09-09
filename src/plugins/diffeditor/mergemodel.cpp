// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergemodel.h"

#include "textdiffmodel.h"

#include <utils/algorithm.h>

#include <optional>

namespace DiffEditor {

ParsedMergeResult parseMergeResult(const QString &text, const QString &placeholderPrefix)
{
    ParsedMergeResult result;
    result.placeholderPrefix = placeholderPrefix;
    while (text.contains(result.placeholderPrefix))
        result.placeholderPrefix += '_';
    const QStringList lines = text.split('\n', Qt::KeepEmptyParts);
    QStringList output;
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        int width = 0;
        while (width < line.size() && line.at(width) == '<')
            ++width;
        if (width < 7 || width == line.size() || line.at(width) != ' ') {
            output.append(line);
            continue;
        }
        const auto isMarker = [width](const QString &candidate, QChar marker, bool label) {
            const QString prefix(width, marker);
            return candidate == prefix || (label && candidate.startsWith(prefix + ' '));
        };
        MergeConflict conflict;
        enum { Ours, Base, Theirs } section = Ours;
        int end = i + 1;
        for (; end < lines.size(); ++end) {
            const QString &candidate = lines.at(end);
            if (isMarker(candidate, '<', true))
                break;
            if (section == Ours && isMarker(candidate, '|', true)) {
                conflict.base.emplace();
                section = Base;
            } else if (section != Theirs && isMarker(candidate, '=', false)) {
                section = Theirs;
            } else if (section == Theirs && isMarker(candidate, '>', true)) {
                break;
            } else if (section == Ours) {
                conflict.ours.append(candidate);
            } else if (section == Base) {
                conflict.base->append(candidate);
            } else {
                conflict.theirs.append(candidate);
            }
        }
        if (end == lines.size() || section != Theirs
            || !isMarker(lines.at(end), '>', true)) {
            output.append(line);
            continue;
        }
        conflict.originalText = lines.mid(i, end - i + 1).join('\n');
        output.append(result.placeholderPrefix + QString::number(result.conflicts.size() + 1));
        result.conflicts.append(conflict);
        i = end;
    }
    result.text = output.join('\n');
    return result;
}


namespace {

struct BaseChange
{
    int baseStart = 0; // 0-based, half-open range
    int baseCount = 0;
    QStringList replacement;

    int baseEnd() const { return baseStart + baseCount; }
};

QStringList linesOf(const QString &text)
{
    if (text.isEmpty())
        return {};

    QStringList lines = text.split('\n', Qt::KeepEmptyParts);
    if (text.endsWith('\n'))
        lines.removeLast();
    return lines;
}

QList<BaseChange> changesFrom(const MergeSnapshot &baseSnapshot,
                              const MergeSnapshot &sideSnapshot, bool patience,
                              const std::optional<QFuture<void>> &cancellation)
{
    const QString base = baseSnapshot.exists ? baseSnapshot.text : QString();
    const QString side = sideSnapshot.exists ? sideSnapshot.text : QString();
    const TextDiff diff = computeTextDiff(base, side, patience ? Utils::Differ::Patience : Utils::Differ::Default,
                                          cancellation);
    const ChunkData &chunk = diff.chunk;
    const QStringList baseLines = linesOf(base);
    const QStringList sideLines = linesOf(side);

    QList<BaseChange> changes;
    int baseLine = 0;
    int sideLine = 0;
    std::optional<BaseChange> current;

    const auto finish = [&] {
        if (current) {
            changes.append(*current);
            current.reset();
        }
    };

    for (const RowData &row : chunk.rows) {
        if (cancellation && cancellation->isCanceled())
            return {};
        const TextLineData &baseLineData = row.line[LeftSide];
        const TextLineData &sideLineData = row.line[RightSide];
        const bool hasBaseLine = baseLineData.textLineType == TextLineData::TextLine;
        const bool hasSideLine = sideLineData.textLineType == TextLineData::TextLine;
        const bool basePhantomLine = hasBaseLine && base.endsWith('\n')
                                     && baseLine >= int(baseLines.size())
                                     && baseLineData.text.isEmpty();
        const bool sidePhantomLine = hasSideLine && side.endsWith('\n')
                                     && sideLine >= int(sideLines.size())
                                     && sideLineData.text.isEmpty();

        // A trailing-newline-only difference can produce a non-equal visual
        // row without changing either line's text.
        if (basePhantomLine || sidePhantomLine) {
            finish();
            continue;
        }
        if (row.equal || (hasBaseLine && hasSideLine
                          && baseLineData.text == sideLineData.text)) {
            finish();
            if (hasBaseLine)
                ++baseLine;
            if (hasSideLine)
                ++sideLine;
            continue;
        }

        if (!hasBaseLine && !hasSideLine)
            continue;

        if (!current)
            current = BaseChange{baseLine, 0, {}};

        if (hasBaseLine) {
            ++current->baseCount;
            ++baseLine;
        }
        if (hasSideLine) {
            current->replacement.append(sideLineData.text);
            ++sideLine;
        }
    }
    finish();

    if (changes.isEmpty() && baseSnapshot.exists != sideSnapshot.exists) {
        if (baseSnapshot.exists)
            changes.append({0, int(baseLines.size()), {}});
        else
            changes.append({0, 0, sideLines});
    }

    return changes;
}

bool changesOverlap(const BaseChange &first, const BaseChange &second)
{
    const bool firstInsertion = first.baseCount == 0;
    const bool secondInsertion = second.baseCount == 0;
    if (firstInsertion && secondInsertion)
        return first.baseStart == second.baseStart;
    if (firstInsertion)
        return first.baseStart > second.baseStart && first.baseStart < second.baseEnd();
    if (secondInsertion)
        return second.baseStart > first.baseStart && second.baseStart < first.baseEnd();
    return first.baseStart < second.baseEnd() && second.baseStart < first.baseEnd();
}

bool belongsToGroup(const BaseChange &change, int groupStart, int groupEnd)
{
    if (change.baseCount == 0)
        return change.baseStart >= groupStart && change.baseStart <= groupEnd;
    return change.baseStart <= groupEnd && change.baseEnd() >= groupStart;
}

QStringList sideLinesForRange(const QStringList &baseLines, const QList<BaseChange> &changes,
                              int start, int end,
                              const std::optional<QFuture<void>> &cancellation)
{
    QStringList result;
    int cursor = start;
    for (const BaseChange &change : changes) {
        if (cancellation && cancellation->isCanceled())
            return {};
        if (!belongsToGroup(change, start, end))
            continue;

        const int changeStart = qBound(start, change.baseStart, end);
        if (changeStart > cursor)
            result.append(baseLines.mid(cursor, changeStart - cursor));
        result.append(change.replacement);
        cursor = qMax(cursor, qMin(end, change.baseEnd()));
    }
    if (cursor < end)
        result.append(baseLines.mid(cursor, end - cursor));
    return result;
}

int sideLineOffset(const QList<BaseChange> &changes, int baseLine,
                   const std::optional<QFuture<void>> &cancellation)
{
    int result = 0;
    int cursor = 0;
    for (const BaseChange &change : changes) {
        if (cancellation && cancellation->isCanceled())
            return 0;
        if (change.baseStart >= baseLine)
            break; // an insertion at the region's start belongs to the region
        result += change.baseStart - cursor;
        result += change.replacement.size();
        cursor = change.baseEnd();
    }
    return result + baseLine - cursor;
}

bool hasConflictingChanges(const QList<BaseChange> &ours, const QList<BaseChange> &theirs,
                           const std::optional<QFuture<void>> &cancellation)
{
    for (const BaseChange &oursChange : ours) {
        for (const BaseChange &theirsChange : theirs) {
            if (cancellation && cancellation->isCanceled())
                return false;
            if (changesOverlap(oursChange, theirsChange))
                return true;
        }
    }
    return false;
}

} // namespace

MergeModel computeMergeModel(const MergeSnapshot &base, const MergeSnapshot &ours,
                             const MergeSnapshot &theirs, bool patience,
                             const std::optional<QFuture<void>> &cancellation)
{
    const QString baseText = base.exists ? base.text : QString();
    const QString oursText = ours.exists ? ours.text : QString();
    const QString theirsText = theirs.exists ? theirs.text : QString();
    const QStringList baseLines = linesOf(baseText);

    const QList<BaseChange> oursChanges = changesFrom(base, ours, patience, cancellation);
    const QList<BaseChange> theirsChanges = changesFrom(base, theirs, patience, cancellation);

    if (cancellation && cancellation->isCanceled())
        return {};

    struct TaggedChange
    {
        BaseChange change;
        bool ours = false;
    };

    QList<TaggedChange> allChanges;
    for (const BaseChange &change : oursChanges) {
        if (cancellation && cancellation->isCanceled())
            return {};
        allChanges.append({change, true});
    }
    for (const BaseChange &change : theirsChanges) {
        if (cancellation && cancellation->isCanceled())
            return {};
        allChanges.append({change, false});
    }
    Utils::sort(allChanges, [](const TaggedChange &first, const TaggedChange &second) {
        if (first.change.baseStart != second.change.baseStart)
            return first.change.baseStart < second.change.baseStart;
        if (first.change.baseCount != second.change.baseCount)
            return first.change.baseCount > second.change.baseCount;
        return first.ours && !second.ours;
    });

    MergeModel model;
    model.computed = true;
    model.baseEndsWithNewline = baseText.endsWith('\n');
    model.oursEndsWithNewline = oursText.endsWith('\n');
    model.theirsEndsWithNewline = theirsText.endsWith('\n');

    int index = 0;
    while (index < allChanges.size()) {
        if (cancellation && cancellation->isCanceled())
            return {};
        const int groupStart = allChanges.at(index).change.baseStart;
        int groupEnd = allChanges.at(index).change.baseEnd();
        int next = index + 1;
        while (next < allChanges.size()) {
            if (cancellation && cancellation->isCanceled())
                return {};
            const BaseChange &change = allChanges.at(next).change;
            if (change.baseStart > groupEnd)
                break;
            groupEnd = qMax(groupEnd, change.baseEnd());
            ++next;
        }

        QList<BaseChange> groupOurs;
        QList<BaseChange> groupTheirs;
        for (int i = index; i < next; ++i) {
            if (cancellation && cancellation->isCanceled())
                return {};
            const TaggedChange &tagged = allChanges.at(i);
            (tagged.ours ? groupOurs : groupTheirs).append(tagged.change);
        }

        const QStringList oursLines = sideLinesForRange(baseLines, groupOurs,
                                                         groupStart, groupEnd, cancellation);
        if (cancellation && cancellation->isCanceled())
            return {};
        const QStringList theirsLines = sideLinesForRange(baseLines, groupTheirs,
                                                           groupStart, groupEnd, cancellation);
        if (cancellation && cancellation->isCanceled())
            return {};

        MergeRegion region;
        region.id = model.regions.size();
        region.base = {groupStart + 1, groupEnd - groupStart};
        region.baseLines = baseLines.mid(groupStart, groupEnd - groupStart);
        region.oursLines = oursLines;
        region.theirsLines = theirsLines;
        region.ours = {sideLineOffset(oursChanges, groupStart, cancellation) + 1,
                       int(oursLines.size())};
        region.theirs = {sideLineOffset(theirsChanges, groupStart, cancellation) + 1,
                         int(theirsLines.size())};
        if (cancellation && cancellation->isCanceled())
            return {};

        if (groupOurs.isEmpty()) {
            region.kind = MergeRegionKind::TheirsOnly;
        } else if (groupTheirs.isEmpty()) {
            region.kind = MergeRegionKind::OursOnly;
        } else if (ours.exists != theirs.exists) {
            region.kind = MergeRegionKind::Conflict;
        } else if (oursLines == theirsLines) {
            region.kind = MergeRegionKind::BothSame;
        } else if (hasConflictingChanges(groupOurs, groupTheirs, cancellation)) {
            region.kind = MergeRegionKind::Conflict;
        } else {
            region.kind = MergeRegionKind::AutoMerged;
        }
        if (cancellation && cancellation->isCanceled())
            return {};

        model.regions.append(region);
        index = next;
    }

    return model;
}

MergeModel computeMergeModel(const QString &base, const QString &ours, const QString &theirs,
                             bool patience)
{
    return computeMergeModel(MergeSnapshot{true, base, "BASE"},
                             MergeSnapshot{true, ours, "OURS"},
                             MergeSnapshot{true, theirs, "THEIRS"}, patience);
}

} // namespace DiffEditor
