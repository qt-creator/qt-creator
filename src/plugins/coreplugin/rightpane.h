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

#include "id.h"

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Core {

class IMode;
class RightPaneWidget;

class CORE_EXPORT RightPanePlaceHolder : public QWidget
{
    friend class Core::RightPaneWidget;
    Q_OBJECT

public:
    explicit RightPanePlaceHolder(Id mode, QWidget *parent = 0);
    ~RightPanePlaceHolder();
    static RightPanePlaceHolder *current();

private:
    void currentModeChanged(Id mode);
    void applyStoredSize(int width);
    Id m_mode;
    static RightPanePlaceHolder* m_current;
};

class CORE_EXPORT RightPaneWidget : public QWidget
{
    Q_OBJECT

public:
    RightPaneWidget();
    ~RightPaneWidget();

    void saveSettings(QSettings *settings);
    void readSettings(QSettings *settings);

    bool isShown();
    void setShown(bool b);

    static RightPaneWidget *instance();

    void setWidget(QWidget *widget);

    int storedWidth();

protected:
    void resizeEvent(QResizeEvent *);

private:
    void clearWidget();
    bool m_shown;
    int m_width;
    QPointer<QWidget> m_widget;
    static RightPaneWidget *m_instance;
};

} // namespace Core
