// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {
class QtcSettings;
}

namespace Core {

class RightPaneWidget;

class CORE_EXPORT RightPanePlaceHolder : public QWidget
{
    friend class Core::RightPaneWidget;
    Q_OBJECT

public:
    explicit RightPanePlaceHolder(Utils::Id mode, QWidget *parent = nullptr);
    ~RightPanePlaceHolder() override;
    static RightPanePlaceHolder *current();

private:
    void currentModeChanged(Utils::Id mode);
    void applyStoredSize(int width);
    Utils::Id m_mode;
    static RightPanePlaceHolder* m_current;
};

class CORE_EXPORT RightPaneWidget : public QWidget
{
    Q_OBJECT

public:
    RightPaneWidget();
    ~RightPaneWidget() override;

    void saveSettings(Utils::QtcSettings *settings);
    void readSettings(Utils::QtcSettings *settings);

    bool isShown() const;
    void setShown(bool b);

    static RightPaneWidget *instance();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    int storedWidth() const;

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    void clearWidget();
    bool m_shown = true;
    int m_width = 0;
    QPointer<QWidget> m_widget;
    static RightPaneWidget *m_instance;
};

} // namespace Core
