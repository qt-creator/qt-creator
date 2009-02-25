/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FUTUREPROGRESS_H
#define FUTUREPROGRESS_H

#include <coreplugin/core_global.h>

#include <QtCore/QString>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <QtGui/QAction>
#include <QtGui/QProgressBar>
#include <QtGui/QMouseEvent>
#include <QtGui/QHBoxLayout>

class ProgressBar;

namespace Core {

class CORE_EXPORT FutureProgress : public QWidget
{
    Q_OBJECT

public:
    FutureProgress(QWidget *parent = 0);
    ~FutureProgress();

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const;

    void setTitle(const QString &title);
    QString title() const;

    bool hasError() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const { return m_widget; }

signals:
    void clicked();
    void finished();

protected:
    void mousePressEvent(QMouseEvent *event);

private slots:
    void cancel();
    void setStarted();
    void setFinished();
    void setProgressRange(int min, int max);
    void setProgressValue(int val);
    void setProgressText(const QString &text);

private:
    QFutureWatcher<void> m_watcher;
    ProgressBar *m_progress;
    QWidget *m_widget;
    QHBoxLayout *m_widgetLayout;
};

} // namespace Core

#endif // FUTUREPROGRESS_H
