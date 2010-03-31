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

#ifndef WELCOMEMODE_H
#define WELCOMEMODE_H

#include "welcome_global.h"

#include <coreplugin/imode.h>

QT_BEGIN_NAMESPACE
class QWidget;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
    class IWelcomePage;
}
namespace Welcome {

struct WelcomeModePrivate;

class WELCOME_EXPORT WelcomeMode : public Core::IMode
{
    Q_OBJECT

public:
    WelcomeMode();
    ~WelcomeMode();

    // IMode
    QString displayName() const;
    QIcon icon() const;
    int priority() const;
    QWidget *widget();
    QString id() const;
    QList<int> context() const;
    void activated();
    QString contextHelpId() const { return QLatin1String("Qt Creator"); }
    void initPlugins();

private slots:
    void slotFeedback();
    void welcomePluginAdded(QObject*);
    void showClickedPage();

private:
    QToolButton *addPageToolButton(Utils::IWelcomePage *plugin, int position = -1);

    WelcomeModePrivate *m_d;
};

} // namespace Welcome

#endif // WELCOMEMODE_H
