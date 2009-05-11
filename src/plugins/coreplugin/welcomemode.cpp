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

#include <QtGui/QToolBar>
#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QUrl>

#include "ui_welcomemode.h"

namespace Core {
namespace Internal {

struct WelcomeModePrivate
{
    WelcomeModePrivate();

    QWidget *m_widget;
    QWidget *m_welcomePage;
    Ui::WelcomePage ui;

    WelcomeMode::WelcomePageData lastData;
};

WelcomeModePrivate::WelcomeModePrivate()
{
}

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
    m_d->m_widget = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(m_d->m_widget);
    l->setMargin(0);
    l->setSpacing(0);
    l->addWidget(new QToolBar(m_d->m_widget));

    m_d->m_welcomePage = new QWidget(m_d->m_widget);
    m_d->ui.setupUi(m_d->m_welcomePage);
    m_d->ui.sessTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.projTreeWidget->viewport()->setAutoFillBackground(false);
    l->addWidget(m_d->m_welcomePage);

    updateWelcomePage(WelcomePageData());

    QButtonGroup *btnGrp = new QButtonGroup(this);
    btnGrp->addButton(m_d->ui.gettingStartedSectButton, 0);
    btnGrp->addButton(m_d->ui.developSectButton, 1);
    btnGrp->addButton(m_d->ui.communitySectButton, 2);

    connect(btnGrp, SIGNAL(buttonClicked(int)), m_d->ui.stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_d->ui.gettingStartedButton, SIGNAL(clicked()), SIGNAL(requestHelp()));
    connect(m_d->ui.feedbackButton, SIGNAL(clicked()), SLOT(slotFeedback()));
    connect(m_d->ui.restoreSessionButton, SIGNAL(clicked()), SLOT(slotRestoreLastSession()));
    connect(m_d->ui.sessTreeWidget, SIGNAL(activated(QString)), SLOT(slotSessionClicked(QString)));
    connect(m_d->ui.projTreeWidget, SIGNAL(activated(QString)), SLOT(slotProjectClicked(QString)));
}

WelcomeMode::~WelcomeMode()
{
    delete m_d;
}

QString WelcomeMode::name() const
{
    return tr("Welcome");
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
    // Update only if data are modified
    if (welcomePageData == m_d->lastData)
        return;
    m_d->lastData = welcomePageData;

    m_d->m_widget->setUpdatesEnabled(false);
    if (!welcomePageData.previousSession.isEmpty() || !welcomePageData.projectList.isEmpty()) {
        m_d->ui.sessTreeWidget->clear();
        m_d->ui.projTreeWidget->clear();

        if (welcomePageData.sessionList.count() > 1) {
            foreach (const QString &s, welcomePageData.sessionList) {
                QString str = s;
                if (s == welcomePageData.previousSession)
                    str = tr("%1 (last session)").arg(s);
                m_d->ui.sessTreeWidget->addItem(str, s);
            }
            m_d->ui.sessTreeWidget->updateGeometry();
            m_d->ui.sessTreeWidget->show();
        } else {
            m_d->ui.sessTreeWidget->hide();
        }

        typedef QPair<QString, QString> QStringPair;
        foreach (const QStringPair &it, welcomePageData.projectList) {
            QTreeWidgetItem *item = m_d->ui.projTreeWidget->addItem(it.second, it.first);
            const QFileInfo fi(it.first);
            item->setToolTip(1, QDir::toNativeSeparators(fi.absolutePath()));
        }
        m_d->ui.projTreeWidget->updateGeometry();

        m_d->ui.recentSessionsFrame->show();
        m_d->ui.recentProjectsFrame->show();
    } else {
        m_d->ui.recentSessionsFrame->hide();
        m_d->ui.recentProjectsFrame->hide();
    }
    m_d->m_widget->setUpdatesEnabled(true);
}

void WelcomeMode::activateEditMode()
{
    Core::ModeManager *modeManager = ModeManager::instance();
    if (modeManager->currentMode() == this)
        modeManager->activateMode(Core::Constants::MODE_EDIT);
}

void WelcomeMode::slotSessionClicked(const QString &data)
{
    emit requestSession(data);
    activateEditMode();
}

void WelcomeMode::slotProjectClicked(const QString &data)
{
    emit requestProject(data);
    activateEditMode();
}

void WelcomeMode::slotRestoreLastSession()
{
    emit requestSession(m_d->lastData.previousSession);
    activateEditMode();
}

void WelcomeMode::slotFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
            "http://www.qtsoftware.com/forms/feedback-forms/qt-creator-user-feedback/view")));
}

// ---  WelcomeModeButton

WelcomeModeButton::WelcomeModeButton(QWidget *parent) :
        QLabel(parent),
        m_isPressed(false),
        m_isInited(false)
{
    setCursor(QCursor(Qt::PointingHandCursor));
}

void WelcomeModeButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_isPressed = true;
}

void WelcomeModeButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isPressed) {
        m_isPressed = false;
        if (rect().contains(event->pos()))
            emit clicked();
    }
}

void WelcomeModeButton::enterEvent(QEvent *)
{
    if (!m_isInited) {
        m_isInited = true;
        m_text = text();
        m_hoverText = m_text;
        m_hoverText.replace(QLatin1String(".png"), QLatin1String("_hover.png"));
        if (m_text == m_hoverText) {
            m_text.clear();
            m_hoverText.clear();
        }
    }
    if (!m_hoverText.isEmpty())
        setText(m_hoverText);
}

void WelcomeModeButton::leaveEvent(QEvent *)
{
    if (!m_text.isEmpty())
        setText(m_text);
}

// ---  WelcomeModeTreeWidget

WelcomeModeTreeWidget::WelcomeModeTreeWidget(QWidget *parent) :
        QTreeWidget(parent),
        m_bullet(QLatin1String(":/core/images/welcomemode/list_bullet_arrow.png"))
{
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(slotItemClicked(QTreeWidgetItem *)));
}

QSize WelcomeModeTreeWidget::sizeHint() const
{
    return QSize(QTreeWidget::sizeHint().width(), 30 * topLevelItemCount());
}

QTreeWidgetItem *WelcomeModeTreeWidget::addItem(const QString &label, const QString &data)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(this);
    item->setIcon(0, m_bullet);
    item->setSizeHint(0, QSize(24, 30));
    QWidget *lbl = new QLabel(label);
    lbl->setCursor(QCursor(Qt::PointingHandCursor));
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QBoxLayout *lay = new QVBoxLayout;
    lay->setContentsMargins(3, 2, 0, 0);
    lay->addWidget(lbl);
    QWidget *wdg = new QWidget;
    wdg->setLayout(lay);
    setItemWidget(item, 1, wdg);
    item->setData(0, Qt::UserRole, data);
    return item;
}

void WelcomeModeTreeWidget::slotItemClicked(QTreeWidgetItem *item)
{
    emit activated(item->data(0, Qt::UserRole).toString());
}

} // namespace Internal
} // namespace Core
