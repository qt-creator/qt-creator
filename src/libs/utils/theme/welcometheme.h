/****************************************************************************
**
** Copyright (C) 2014 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef WELCOMETHEME_H
#define WELCOMETHEME_H

#include "../utils_global.h"

#include <QStyle>

#define DECLARE_COLOR_PROPERTY(propName) \
  Q_PROPERTY(QColor propName READ propName NOTIFY themeChanged) \
  QColor propName () const;

/*!
 * Expose theme properties to QML (for welcomeplugin)
 */
class QTCREATOR_UTILS_EXPORT WelcomeTheme : public QObject
{
    Q_OBJECT

public:
    WelcomeTheme(QObject *parent);

    DECLARE_COLOR_PROPERTY(textColorNormal)
    DECLARE_COLOR_PROPERTY(textColorHeading)
    DECLARE_COLOR_PROPERTY(backgroundColorNormal)
    DECLARE_COLOR_PROPERTY(dividerColor)
    DECLARE_COLOR_PROPERTY(button_BorderColor)
    DECLARE_COLOR_PROPERTY(button_TextColorNormal)
    DECLARE_COLOR_PROPERTY(button_TextColorPressed)
    DECLARE_COLOR_PROPERTY(link_TextColorNormal)
    DECLARE_COLOR_PROPERTY(link_TextColorActive)
    DECLARE_COLOR_PROPERTY(link_BackgroundColor)
    DECLARE_COLOR_PROPERTY(sideBar_BackgroundColor)
    DECLARE_COLOR_PROPERTY(caption_TextColorNormal)
    DECLARE_COLOR_PROPERTY(projectItem_TextColorFilepath)
    DECLARE_COLOR_PROPERTY(projectItem_BackgroundColorHover)
    DECLARE_COLOR_PROPERTY(sessionItem_BackgroundColorNormal)
    DECLARE_COLOR_PROPERTY(sessionItemExpanded_BackgroundColorNormal)
    DECLARE_COLOR_PROPERTY(sessionItemExpanded_BackgroundColorHover)
    DECLARE_COLOR_PROPERTY(sessionItem_BackgroundColorHover)

    Q_PROPERTY(QGradient button_GradientNormal READ  button_GradientNormal  NOTIFY themeChanged)
    Q_PROPERTY(QGradient button_GradientPressed READ button_GradientPressed NOTIFY themeChanged)

    Q_PROPERTY(QString widgetStyle READ widgetStyle CONSTANT)

    QString widgetStyle() const;

    QGradient button_GradientNormal () const;
    QGradient button_GradientPressed () const;

public slots:
    void notifyThemeChanged();

signals:
    void themeChanged();
};

#endif // WELCOMETHEME_H
