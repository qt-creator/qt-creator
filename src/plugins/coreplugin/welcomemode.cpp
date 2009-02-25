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

#include "welcomemode.h"
#include "coreconstants.h"
#include "uniqueidmanager.h"
#include "modemanager.h"

#if !defined(QT_NO_WEBKIT)
#include <QtWebKit/QWebView>
#include <QtGui/QApplication>
#include <QtCore/QFileInfo>
#else
#include <QtGui/QLabel>
#endif
#include <QtGui/QToolBar>
#include <QtGui/QDesktopServices>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QUrl>

namespace Core {
namespace Internal {

static QString readFile(const QString &name)
{
    QFile f(name);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open %s: %s", name.toUtf8().constData(), f.errorString().toUtf8().constData());
        return QString();
    }
    QTextStream ts(&f);
    return ts.readAll();
}

struct WelcomeModePrivate
{
    WelcomeModePrivate();

    QWidget *m_widget;
#if !defined(QT_NO_WEBKIT)
    QWebView *m_webview;
#else
    QLabel *m_label;
#endif

    WelcomeMode::WelcomePageData lastData;

    const QString m_htmlTemplate;
    const QString m_sessionHtmlTemplate;
    const QString m_projectHtmlTemplate;
    const QUrl m_baseUrl;
};

WelcomeModePrivate::WelcomeModePrivate() :
    m_widget(new QWidget),
#if !defined(QT_NO_WEBKIT)
    m_webview(new QWebView),
#else
    m_label(new QLabel),
#endif
    m_htmlTemplate(readFile(QLatin1String(":/core/html/welcome.html"))),
    m_sessionHtmlTemplate(readFile(QLatin1String(":/core/html/recent_sessions.html"))),
    m_projectHtmlTemplate(readFile(QLatin1String(":/core/html/recent_projects.html"))),
    m_baseUrl(QUrl(QLatin1String("qrc:/core/html/welcome.html")))
{
}

#if defined(QT_NO_WEBKIT)

const char *LABEL = "<center><table><tr><td><img src=\":/core/html/images/product_logo.png\"/></td><td width=300>"
                    "<h2><br/><br/>Welcome</h2><p> Qt Creator is an intuitive, modern cross platform IDE that enables "
                    "developers to create graphically appealing applications for desktop, "
                    "embedded, and mobile devices. "
                    "<p><font color=\"red\">(This startup page lacks features due to disabled webkit support)</font>"
                    "</td></tr></table>";

#endif
// ---  WelcomePageData

bool WelcomeMode::WelcomePageData::operator==(const WelcomePageData &rhs) const
{
    return previousSession == rhs.previousSession
        && activeSession   == rhs.activeSession
        && sessionList     == rhs.sessionList
        && projectList     == rhs.projectList;
}

bool WelcomeMode::WelcomePageData::operator!=(const WelcomePageData &rhs) const
{
    return previousSession != rhs.previousSession
        || activeSession   != rhs.activeSession
        || sessionList     != rhs.sessionList
        || projectList     != rhs.projectList;
}

QDebug operator<<(QDebug dgb, const WelcomeMode::WelcomePageData &d)
{
    dgb.nospace() << "PreviousSession=" << d.previousSession
        << " activeSession=" << d.activeSession
        << " sessionList=" << d.sessionList
        << " projectList=" << d.projectList;
    return dgb;
}

// ---  WelcomeMode
WelcomeMode::WelcomeMode() :
    m_d(new WelcomeModePrivate)
{
    QVBoxLayout *l = new QVBoxLayout(m_d->m_widget);
    l->setMargin(0);
    l->setSpacing(0);
    l->addWidget(new QToolBar(m_d->m_widget));
#if !defined(QT_NO_WEBKIT)
    connect(m_d->m_webview, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)));

    WelcomePageData welcomePageData;
    updateWelcomePage(welcomePageData);

    l->addWidget(m_d->m_webview);
    m_d->m_webview->setAcceptDrops(false);
    m_d->m_webview->settings()->setAttribute(QWebSettings::PluginsEnabled, false);
    m_d->m_webview->settings()->setAttribute(QWebSettings::JavaEnabled, false);

#else
    m_d->m_label->setWordWrap(true);
    m_d->m_label->setAlignment(Qt::AlignCenter);
    m_d->m_label->setText(LABEL);
    l->addWidget(m_d->m_label);
#endif
}

WelcomeMode::~WelcomeMode()
{
    delete m_d;
}

QString WelcomeMode::name() const
{
    return QLatin1String("Welcome");
}

QIcon WelcomeMode::icon() const
{
    return QIcon(QLatin1String(":/core/images/qtcreator_logo_32.png"));
}

int WelcomeMode::priority() const
{
    return Constants::P_MODE_WELCOME;
}

QWidget* WelcomeMode::widget()
{
    return m_d->m_widget;
}

const char* WelcomeMode::uniqueModeName() const
{
    return Constants::MODE_WELCOME;
}

QList<int> WelcomeMode::context() const
{
    static QList<int> contexts = QList<int>()
                                 << UniqueIDManager::instance()->uniqueIdentifier(Constants::C_WELCOME_MODE);
    return contexts;
}

void WelcomeMode::updateWelcomePage(const WelcomePageData &welcomePageData)
{
// should really only modify the DOM tree

#if defined(QT_NO_WEBKIT)
    Q_UNUSED(welcomePageData);
#else

    // Update only if data are modified
    if (welcomePageData == m_d->lastData)
        return;
    m_d->lastData = welcomePageData;

    QString html = m_d->m_htmlTemplate;

    if (!welcomePageData.previousSession.isEmpty() || !welcomePageData.projectList.isEmpty()) {
        QString sessionHtml = m_d->m_sessionHtmlTemplate;
        sessionHtml.replace(QLatin1String("LAST_SESSION"), welcomePageData.previousSession);

        if (welcomePageData.sessionList.count() > 1) {
            QString sessions;
            foreach (QString s, welcomePageData.sessionList) {
                QString last;
                if (s == welcomePageData.previousSession)
                    last = tr(" (last session)");
                sessions += QString::fromLatin1("<li><p><a href=\"gh-session:%1\">%2%3</a></p></li>").arg(s, s, last);
            }
            sessionHtml.replace(QLatin1String("<!-- RECENT SESSIONS LIST -->"), sessions);
        }
        html.replace(QLatin1String("<!-- RECENT SESSIONS -->"), sessionHtml);

        QString projectHtml = m_d->m_projectHtmlTemplate;
        {
            QString projects;
            QTextStream str(&projects);
            foreach (const QString &s, welcomePageData.projectList) {
                const QFileInfo fi(s);
                str << "<li><p><a href=\"gh-project:" << s << "\" title=\""
                    << fi.absolutePath() << "\">" << fi.fileName() << "</a></p></li>\n";
            }
            projectHtml.replace(QLatin1String("<!-- RECENT PROJECTS LIST -->"), projects);
        }
        html.replace(QLatin1String("<!-- RECENT PROJECTS -->"), projectHtml);
    }

    m_d->m_webview->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    m_d->m_webview->setHtml(html, m_d->m_baseUrl);
#endif
}

void WelcomeMode::linkClicked(const QUrl &url)
{
    QString scheme = url.scheme();
    Core::ModeManager *modeManager = ModeManager::instance();
    if (scheme.startsWith(QLatin1String("gh"))) {
        QString s = url.toString(QUrl::RemoveScheme);
        if (scheme == QLatin1String("gh")) {
            emit requestHelp(s);
        } else if (scheme == QLatin1String("gh-project")) {
            emit requestProject(s);
            if (modeManager->currentMode() == this)
                modeManager->activateMode(Core::Constants::MODE_EDIT);
        } else if (scheme == QLatin1String("gh-session")) {
            emit requestSession(s);
            if (modeManager->currentMode() == this)
                modeManager->activateMode(Core::Constants::MODE_EDIT);
        }
    } else {
        QDesktopServices::openUrl(url);
    }
}

} // namespace Internal
} // namespace Core
