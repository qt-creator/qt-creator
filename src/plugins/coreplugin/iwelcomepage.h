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

#include "core_global.h"

#include "id.h"

#include <QWidget>
#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT IWelcomePage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(int priority READ priority CONSTANT)

public:
    IWelcomePage();
    virtual ~IWelcomePage();

    virtual QString title() const = 0;
    virtual int priority() const { return 0; }
    virtual Core::Id id() const = 0;
    virtual QWidget *createWidget() const = 0;
};

class WelcomePageButtonPrivate;

class CORE_EXPORT WelcomePageFrame : public QWidget
{
public:
    WelcomePageFrame(QWidget *parent);

    void paintEvent(QPaintEvent *event) override;
};

class CORE_EXPORT WelcomePageButton : public WelcomePageFrame
{
public:
    WelcomePageButton(QWidget *parent);
    ~WelcomePageButton();

    void mousePressEvent(QMouseEvent *) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void setText(const QString &text);
    void setIcon(const QPixmap &pixmap);
    void setOnClicked(const std::function<void ()> &value);
    void setActiveChecker(const std::function<bool ()> &value);
    void recheckActive();
    void click();

private:
    WelcomePageButtonPrivate *d;
};

} // Core
