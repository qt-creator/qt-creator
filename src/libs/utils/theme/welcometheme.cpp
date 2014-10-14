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

#include "welcometheme.h"

#include "theme.h"

using namespace Utils;

#define IMPL_COLOR_PROPERTY(propName, enumName) \
  QColor WelcomeTheme::propName () const { \
      return creatorTheme()->color(Theme::enumName); \
  }

#define GRADIENT(x) \
  QGradient grad; \
  grad.setStops(creatorTheme()->gradient(Theme::x)); \
  return grad;

WelcomeTheme::WelcomeTheme(QObject *parent)
  : QObject(parent)
{
}

QString WelcomeTheme::widgetStyle() const
{
  switch (creatorTheme()->widgetStyle()) {
  case Theme::StyleDefault:
      return QLatin1String("default");
  case Theme::StyleFlat:
      return QLatin1String("flat");
  }
  return QString::null;
}

void WelcomeTheme::notifyThemeChanged()
{
    emit themeChanged();
}

IMPL_COLOR_PROPERTY(textColorNormal        ,       Welcome_TextColorNormal        )
IMPL_COLOR_PROPERTY(textColorHeading       ,       Welcome_TextColorHeading       )
IMPL_COLOR_PROPERTY(backgroundColorNormal  ,       Welcome_BackgroundColorNormal  )
IMPL_COLOR_PROPERTY(dividerColor           ,       Welcome_DividerColor           )
IMPL_COLOR_PROPERTY(button_BorderColor     ,       Welcome_Button_BorderColor     )
IMPL_COLOR_PROPERTY(button_TextColorNormal ,       Welcome_Button_TextColorNormal )
IMPL_COLOR_PROPERTY(button_TextColorPressed,       Welcome_Button_TextColorPressed)
IMPL_COLOR_PROPERTY(link_TextColorNormal   ,       Welcome_Link_TextColorNormal   )
IMPL_COLOR_PROPERTY(link_TextColorActive   ,       Welcome_Link_TextColorActive   )
IMPL_COLOR_PROPERTY(link_BackgroundColor   ,       Welcome_Link_BackgroundColor   )
IMPL_COLOR_PROPERTY(caption_TextColorNormal,       Welcome_Caption_TextColorNormal)
IMPL_COLOR_PROPERTY(sideBar_BackgroundColor,       Welcome_SideBar_BackgroundColor)
IMPL_COLOR_PROPERTY(projectItem_TextColorFilepath, Welcome_ProjectItem_TextColorFilepath)
IMPL_COLOR_PROPERTY(projectItem_BackgroundColorHover, Welcome_ProjectItem_BackgroundColorHover)
IMPL_COLOR_PROPERTY(sessionItem_BackgroundColorNormal, Welcome_SessionItem_BackgroundColorNormal)
IMPL_COLOR_PROPERTY(sessionItemExpanded_BackgroundColor, Welcome_SessionItemExpanded_BackgroundColor)
IMPL_COLOR_PROPERTY(sessionItem_BackgroundColorHover, Welcome_SessionItem_BackgroundColorHover)

QGradient WelcomeTheme::button_GradientNormal () const  { GRADIENT(Welcome_Button_GradientNormal); }
QGradient WelcomeTheme::button_GradientPressed () const { GRADIENT(Welcome_Button_GradientPressed); }
