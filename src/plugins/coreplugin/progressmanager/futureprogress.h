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

#ifndef FUTUREPROGRESS_H
#define FUTUREPROGRESS_H

#include <coreplugin/core_global.h>

#include <QtCore/QString>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QProgressBar;
class QHBoxLayout;
QT_END_NAMESPACE

class ProgressBar;

namespace Core {

    class FadeWidgetHack : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
    public:
        FadeWidgetHack(QWidget *parent):QWidget(parent), m_opacity(0){
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }
        void paintEvent(QPaintEvent *);

        void setOpacity(float o) { m_opacity = o; update(); }
        float opacity() const { return m_opacity; }

    private:
        float m_opacity;
    };

class CORE_EXPORT FutureProgress : public QWidget
{
    Q_OBJECT

public:
    FutureProgress(QWidget *parent = 0);
    ~FutureProgress();

    bool eventFilter(QObject *object, QEvent *);

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const;

    void setTitle(const QString &title);
    QString title() const;

    void setType(const QString &type) {m_type = type; }
    QString type() const { return m_type; }

    void setKeepOnFinish(bool keep) { m_keep = keep; }
    bool keepOnFinish() const { return m_keep; }

    bool hasError() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const { return m_widget; }

signals:
    void clicked();
    void finished();
    void removeMe();

protected:
    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *);

private slots:
    void updateToolTip(const QString &);
    void cancel();
    void setStarted();
    void setFinished();
    void setProgressRange(int min, int max);
    void setProgressValue(int val);
    void setProgressText(const QString &text);
    void fadeAway();

private:
    QFutureWatcher<void> m_watcher;
    ProgressBar *m_progress;
    QWidget *m_widget;
    QHBoxLayout *m_widgetLayout;
    QString m_type;
    bool m_keep;
    bool m_waitingForUserInteraction;
    FadeWidgetHack *m_faderWidget;

};

} // namespace Core

#endif // FUTUREPROGRESS_H
