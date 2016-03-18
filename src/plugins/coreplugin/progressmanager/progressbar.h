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

#include <QString>
#include <QWidget>

namespace Core {
namespace Internal {

class ProgressBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(float cancelButtonFader READ cancelButtonFader WRITE setCancelButtonFader)

public:
    explicit ProgressBar(QWidget *parent = 0);
    ~ProgressBar();

    QString title() const;
    void setTitle(const QString &title);
    void setTitleVisible(bool visible);
    bool isTitleVisible() const;
    void setSeparatorVisible(bool visible);
    bool isSeparatorVisible() const;
    void setCancelEnabled(bool enabled);
    bool isCancelEnabled() const;
    void setError(bool on);
    bool hasError() const;
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int value() const { return m_value; }
    bool finished() const { return m_finished; }
    void reset();
    void setRange(int minimum, int maximum);
    void setValue(int value);
    void setFinished(bool b);
    float cancelButtonFader() { return m_cancelButtonFader; }
    void setCancelButtonFader(float value) { update(); m_cancelButtonFader= value;}
    bool event(QEvent *);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    QFont titleFont() const;

    QString m_text;
    QString m_title;
    bool m_titleVisible;
    bool m_separatorVisible;
    bool m_cancelEnabled;
    int m_progressHeight;
    int m_minimum;
    int m_maximum;
    int m_value;
    float m_cancelButtonFader;
    bool m_finished;
    bool m_error;
    QRect m_cancelRect;
};

} // namespace Internal
} // namespace Core
