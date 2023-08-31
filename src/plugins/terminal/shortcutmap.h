// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// COPIED FROM shortcutmap_p.h
#pragma once

#include <QKeySequence>
#include <QObject>

QT_BEGIN_NAMESPACE
class QKeyEvent;
class QObject;
QT_END_NAMESPACE

namespace Terminal::Internal {

struct ShortcutEntry;
class ShortcutMapPrivate;

class ShortcutMap
{
    Q_DECLARE_PRIVATE(ShortcutMap)
public:
    ShortcutMap();
    ~ShortcutMap();

    typedef bool (*ContextMatcher)(QObject *object, Qt::ShortcutContext context);

    int addShortcut(QObject *owner,
                    const QKeySequence &key,
                    Qt::ShortcutContext context,
                    ContextMatcher matcher);
    int removeShortcut(int id, QObject *owner, const QKeySequence &key = QKeySequence());

    QKeySequence::SequenceMatch state();

    bool tryShortcut(QKeyEvent *e);
    bool hasShortcutForKeySequence(const QKeySequence &seq) const;

private:
    void resetState();
    QKeySequence::SequenceMatch nextState(QKeyEvent *e);
    bool dispatchEvent(QKeyEvent *e);

    QKeySequence::SequenceMatch find(QKeyEvent *e, int ignoredModifiers = 0);
    QKeySequence::SequenceMatch matches(const QKeySequence &seq1, const QKeySequence &seq2) const;
    QList<const ShortcutEntry *> matches() const;
    void createNewSequences(QKeyEvent *e, QList<QKeySequence> &ksl, int ignoredModifiers);
    void clearSequence(QList<QKeySequence> &ksl);

    QScopedPointer<ShortcutMapPrivate> d_ptr;
};

} // namespace Terminal::Internal
