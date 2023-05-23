// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Core::Internal {

class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionDialog(QWidget *parent = nullptr);

    void setAutoLoadSession(bool);
    bool autoLoadSession() const;

private:
    void updateActions(const QStringList &sessions);

    QPushButton *m_openButton;
    QPushButton *m_renameButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QCheckBox *m_autoLoadCheckBox;
};

class SessionNameInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionNameInputDialog(QWidget *parent);

    void setActionText(const QString &actionText, const QString &openActionText);
    void setValue(const QString &value);
    QString value() const;
    bool isSwitchToRequested() const;

private:
    QLineEdit *m_newSessionLineEdit = nullptr;
    QPushButton *m_switchToButton = nullptr;
    QPushButton *m_okButton = nullptr;
    bool m_usedSwitchTo = false;
};

} // namespace Core::Internal
