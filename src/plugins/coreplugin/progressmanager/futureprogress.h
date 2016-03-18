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

#include <coreplugin/core_global.h>

#include <QString>
#include <QFuture>
#include <QWidget>

namespace Core {
class Id;
class FutureProgressPrivate;

class CORE_EXPORT FutureProgress : public QWidget
{
    Q_OBJECT

public:
    enum KeepOnFinishType {
        HideOnFinish = 0,
        KeepOnFinishTillUserInteraction = 1,
        KeepOnFinish = 2
    };
    explicit FutureProgress(QWidget *parent = 0);
    virtual ~FutureProgress();

    virtual bool eventFilter(QObject *object, QEvent *);

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const;

    void setTitle(const QString &title);
    QString title() const;

    void setType(Id type);
    Id type() const;

    void setKeepOnFinish(KeepOnFinishType keepType);
    bool keepOnFinish() const;

    bool hasError() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setStatusBarWidget(QWidget *widget);
    QWidget *statusBarWidget() const;

    bool isFading() const;

    QSize sizeHint() const;

signals:
    void clicked();
    void finished();
    void canceled();
    void removeMe();
    void hasErrorChanged();
    void fadeStarted();

    void statusBarWidgetChanged();

protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *);

private:
    void updateToolTip(const QString &);
    void cancel();
    void setStarted();
    void setFinished();
    void setProgressRange(int min, int max);
    void setProgressValue(int val);
    void setProgressText(const QString &text);

    FutureProgressPrivate *d;
};

} // namespace Core
