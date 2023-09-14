// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../actionmanager/commandmappings.h"
#include "ioptionspage.h"

#include <QGridLayout>
#include <QKeySequence>
#include <QPointer>
#include <QPushButton>

#include <array>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {

class Command;

namespace Internal {

struct ShortcutItem
{
    Command *m_cmd;
    QList<QKeySequence> m_keys;
    QTreeWidgetItem *m_item;
};

class ShortcutButton : public QPushButton
{
    Q_OBJECT
public:
    ShortcutButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;

signals:
    void keySequenceChanged(const QKeySequence &sequence);

protected:
    bool eventFilter(QObject *obj, QEvent *evt) override;

private:
    void updateText();
    void handleToggleChange(bool toggleState);

    QString m_checkedText;
    QString m_uncheckedText;
    mutable int m_preferredWidth = -1;
    std::array<int, 4> m_key;
    int m_keyNum = 0;
};

class ShortcutInput : public QObject
{
    Q_OBJECT
public:
    ShortcutInput();
    ~ShortcutInput();

    void addToLayout(QGridLayout *layout, int row);

    void setKeySequence(const QKeySequence &key);
    QKeySequence keySequence() const;

    using ConflictChecker = std::function<bool(QKeySequence)>;
    void setConflictChecker(const ConflictChecker &fun);

signals:
    void changed();
    void showConflictsRequested();

private:
    ConflictChecker m_conflictChecker;
    QPointer<QLabel> m_shortcutLabel;
    QPointer<Utils::FancyLineEdit> m_shortcutEdit;
    QPointer<ShortcutButton> m_shortcutButton;
    QPointer<QLabel> m_warningLabel;
};

class ShortcutSettings final : public IOptionsPage
{
public:
    ShortcutSettings();
};

} // namespace Internal
} // namespace Core
