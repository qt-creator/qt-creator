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

#pragma once

#include <coreplugin/actionmanager/commandmappings.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <QGridLayout>
#include <QKeySequence>
#include <QPointer>
#include <QPushButton>

#include <array>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QLabel;
QT_END_NAMESPACE

namespace Core {

class Command;

namespace Internal {

class ActionManagerPrivate;
class ShortcutSettingsWidget;

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

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<ShortcutSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace Core
