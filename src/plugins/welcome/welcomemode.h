/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef WELCOMEMODE_H
#define WELCOMEMODE_H

#include "welcome_global.h"

#include <coreplugin/imode.h>

#include <QtCore/QObject>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QWidget;
class QUrl;
QT_END_NAMESPACE

namespace Welcome {

struct WelcomeModePrivate;

class WELCOME_EXPORT WelcomeMode : public Core::IMode
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
        QList<QPair<QString, QString> > projectList; // pair of filename, displayname
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

    void updateExamples(const QString& examplePath, const QString& demosPath, const QString &demosPath);

signals:
    void requestProject(const QString &project);
    void requestSession(const QString &session);
    void openHelpPage(const QString& url);
    void openContextHelpPage(const QString& url);
    void manageSessions();

private slots:
    void slotFeedback();
    void slotSessionClicked(const QString &data);
    void slotProjectClicked(const QString &data);
    void slotUrlClicked(const QString &data);
    void slotEnableExampleButton(int);
    void slotOpenExample();
    void slotCreateNewProject();
    void slotNextTip();
    void slotPrevTip();

private:
    void activateEditMode();
    QStringList tipsOfTheDay();

    WelcomeModePrivate *m_d;
};

} // namespace Welcome

#endif // WELCOMEMODE_H
