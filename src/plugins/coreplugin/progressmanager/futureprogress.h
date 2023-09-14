// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>

#include <QString>
#include <QFuture>
#include <QWidget>

namespace Core {
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
    explicit FutureProgress(QWidget *parent = nullptr);
    ~FutureProgress() override;

    bool eventFilter(QObject *object, QEvent *) override;

    void setFuture(const QFuture<void> &future);
    QFuture<void> future() const;

    void setTitle(const QString &title);
    QString title() const;

    void setSubtitle(const QString &subtitle);
    QString subtitle() const;

    void setSubtitleVisibleInStatusBar(bool visible);
    bool isSubtitleVisibleInStatusBar() const;

    void setType(Utils::Id type);
    Utils::Id type() const;

    void setKeepOnFinish(KeepOnFinishType keepType);
    bool keepOnFinish() const;

    bool hasError() const;

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setStatusBarWidget(QWidget *widget);
    QWidget *statusBarWidget() const;

    bool isFading() const;

    QSize sizeHint() const override;

    bool isCancelEnabled() const;
    void setCancelEnabled(bool enabled);

signals:
    void clicked();
    void finished();
    void canceled();
    void removeMe();
    void hasErrorChanged();
    void fadeStarted();

    void statusBarWidgetChanged();
    void subtitleInStatusBarChanged();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

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
