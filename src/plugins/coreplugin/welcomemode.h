/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef WELCOMEMODE_H
#define WELCOMEMODE_H

#include <coreplugin/imode.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QWidget;
class QUrl;
class QLabel;
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
    QWidget* widget();
    const char* uniqueModeName() const;
    QList<int> context() const;
    void activated();
    QString contextHelpId() const
    { return QLatin1String("Qt Creator"); }

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
