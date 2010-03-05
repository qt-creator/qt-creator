/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROGRESSPIE_H
#define PROGRESSPIE_H

#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtGui/QProgressBar>
#include <QtGui/QMouseEvent>

class ProgressBar : public QWidget
{
    Q_OBJECT
public:
    ProgressBar(QWidget *parent = 0);
    ~ProgressBar();

    QString title() const;
    void setTitle(const QString &title);
    // TODO rename setError
    void setError(bool on);
    bool hasError() const;
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);

    int minimum() const { return m_minimum; }
    int maximum() const { return m_maximum; }
    int value() const { return m_value; }
    void reset();
    void setRange(int minimum, int maximum);
    void setValue(int value);
signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event);

private:
    QImage bar;
    QString m_text;
    QString m_title;
    bool m_error;
    int m_progressHeight;

    int m_minimum;
    int m_maximum;
    int m_value;
};

#endif // PROGRESSPIE_H
