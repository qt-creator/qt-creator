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

#ifndef WELCOMEMODE_H
#define WELCOMEMODE_H

#include <coreplugin/imode.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QWidget;
class QUrl;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

struct WelcomeModePrivate;

class CORE_EXPORT WelcomeMode : public Core::IMode
{
    Q_OBJECT

public:
    WelcomeMode();
    ~WelcomeMode();

    struct WelcomePageData{
        bool operator==(const WelcomePageData &rhs) const;
        bool operator!=(const WelcomePageData &rhs) const;

        QString previousSession;
        QString activeSession;
        QStringList sessionList;
        QStringList projectList;
    };

    void updateWelcomePage(const WelcomePageData &welcomePageData);

    // IMode
    QString name() const;
    QIcon icon() const;
    int priority() const;
    QWidget *widget();
    const char *uniqueModeName() const;
    QList<int> context() const;
    void activated();
    QString contextHelpId() const { return QLatin1String("Qt Creator"); }

signals:
    void requestProject(const QString &project);
    void requestSession(const QString &session);
    void requestHelp(const QString &help);

private slots:
    void linkClicked(const QUrl &url);

private:
    WelcomeModePrivate *m_d;
};

} // namespace Internal
} // namespace Core

#endif // WELCOMEMODE_H
