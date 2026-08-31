// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QLabel;
class QProgressBar;
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
    bool finished() const { return m_finished; }
    void setFinished(bool b);
    QProgressBar *progressBar() const;

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
};

} // namespace Core::Internal
