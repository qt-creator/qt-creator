// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils { class QtcProgressBar; }

namespace Core::Internal {

class ProgressBar : public QWidget
{
    Q_OBJECT

public:
    enum Role {
        Default,
        Compact,
    };

    explicit ProgressBar(Role role = Default, QWidget *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);
    void setSubtitle(const QString &subtitle);
    QString subtitle() const;
    void setCancelEnabled(bool enabled);
    bool isCancelEnabled() const;
    void setError(bool on);
    bool hasError() const;
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int value() const { return m_value; }
    bool finished() const { return m_finished; }
    void reset();
    void setRange(int minimum, int maximum);
    void setValue(int value);
    void setFinished(bool b);

signals:
    void clicked();

private:
    void updateColor();
    void updateCancelButton();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_subtitleLabel = nullptr;
    Utils::QtcProgressBar *m_progressBar = nullptr;
    QAbstractButton *m_cancelButton = nullptr;

    bool m_cancelEnabled = true;
    bool m_finished = false;
    bool m_error = false;
    int m_minimum = 1;
    int m_maximum = 100;
    int m_value = 1;
};

} // namespace Core::Internal
