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

#include "projectwelcomepage.h"
#include "session.h"
#include "sessionmodel.h"
#include "projectexplorer.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QAbstractItemDelegate>
#include <QAction>
#include <QBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLabel>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>

using namespace Core;
using namespace Utils;

const int LINK_HEIGHT = 35;
const int SESSION_LINE_HEIGHT = 30;
const char PROJECT_BASE_ID[] = "Welcome.OpenRecentProject";

namespace ProjectExplorer {
namespace Internal {

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::recentProjectsChanged,
            this, &ProjectModel::resetProjects);
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return ProjectExplorerPlugin::recentProjects().count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    QPair<QString,QString> data = ProjectExplorerPlugin::recentProjects().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.second;
    case Qt::ToolTipRole:
        return data.first;
    case FilePathRole:
        return data.first;
    case PrettyFilePathRole:
        return Utils::withTildeHomePath(data.first);
    case ShortcutRole: {
        const Id projectBase = PROJECT_BASE_ID;
        if (Command *cmd = ActionManager::command(projectBase.withSuffix(index.row() + 1)))
            return cmd->keySequence().toString(QKeySequence::NativeText);
        return QVariant();
    }
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{
        {Qt::DisplayRole, "displayName"},
        {FilePathRole, "filePath"},
        {PrettyFilePathRole, "prettyFilePath"}
    };

    return extraRoles;
}

void ProjectModel::resetProjects()
{
    beginResetModel();
    endResetModel();
}

///////////////////

ProjectWelcomePage::ProjectWelcomePage()
{
    const int actionsCount = 9;
    Context welcomeContext(Core::Constants::C_WELCOME_MODE);

    const Id projectBase = PROJECT_BASE_ID;
    const Id sessionBase = SESSION_BASE_ID;
    for (int i = 1; i <= actionsCount; ++i) {
        auto act = new QAction(tr("Open Session #%1").arg(i), this);
        Command *cmd = ActionManager::registerAction(act, sessionBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence((UseMacShortcuts ? tr("Ctrl+Meta+%1") : tr("Ctrl+Alt+%1")).arg(i)));
        connect(act, &QAction::triggered, this, [this, i] { openSessionAt(i - 1); });

        act = new QAction(tr("Open Recent Project #%1").arg(i), this);
        cmd = ActionManager::registerAction(act, projectBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+%1").arg(i)));
        connect(act, &QAction::triggered, this, [this, i] { openProjectAt(i - 1); });
    }
}

Core::Id ProjectWelcomePage::id() const
{
    return "Develop";
}

void ProjectWelcomePage::reloadWelcomeScreenData() const
{
    if (m_sessionModel)
        m_sessionModel->resetSessions();
    if (m_projectModel)
        m_projectModel->resetProjects();
}

void ProjectWelcomePage::newProject()
{
    ProjectExplorerPlugin::openNewProjectDialog();
}

void ProjectWelcomePage::openProject()
{
    ProjectExplorerPlugin::openOpenProjectDialog();
}

void ProjectWelcomePage::openSessionAt(int index)
{
    QTC_ASSERT(m_sessionModel, return);
    m_sessionModel->switchToSession(m_sessionModel->sessionAt(index));
}

void ProjectWelcomePage::openProjectAt(int index)
{
    QTC_ASSERT(m_projectModel, return);
    const QString projectFile = m_projectModel->data(m_projectModel->index(index, 0),
                                                     ProjectModel::FilePathRole).toString();
    ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
}

///////////////////

static QColor themeColor(Theme::Color role)
{
    return Utils::creatorTheme()->color(role);
}

static QFont sizedFont(int size, const QWidget *widget, bool underline = false)
{
    QFont f = widget->font();
    f.setPixelSize(size);
    f.setUnderline(underline);
    return f;
}

static QPixmap pixmap(const QString &id, const Theme::Color &color)
{
    const QString fileName = QString(":/welcome/images/%1.png").arg(id);
    return Icon({{fileName, color}}, Icon::Tint).pixmap();
}

class BaseDelegate : public QAbstractItemDelegate
{
protected:
    virtual QString entryType() = 0;
    virtual QRect toolTipArea(const QRect &itemRect, const QModelIndex &) const
    {
        return itemRect;
    }
    virtual int shortcutRole() const = 0;

    bool helpEvent(QHelpEvent *ev, QAbstractItemView *view,
                   const QStyleOptionViewItem &option, const QModelIndex &idx) final
    {
        if (!toolTipArea(option.rect, idx).contains(ev->pos())) {
            QToolTip::hideText();
            return false;
        }

        QString shortcut = idx.data(shortcutRole()).toString();

        QString name = idx.data(Qt::DisplayRole).toString();
        QString tooltipText;
        const QString type = entryType();
        if (shortcut.isEmpty())
            tooltipText = ProjectWelcomePage::tr("Open %1 \"%2\"").arg(type, name);
        else
            tooltipText = ProjectWelcomePage::tr("Open %1 \"%2\" (%3)").arg(type, name, shortcut);

        if (tooltipText.isEmpty())
            return false;

        QToolTip::showText(ev->globalPos(), tooltipText, view);
        return true;
    }
};

class SessionDelegate : public BaseDelegate
{
protected:
    QString entryType() override
    {
        return ProjectWelcomePage::tr("session", "Appears in \"Open session <name>\"");
    }
    QRect toolTipArea(const QRect &itemRect, const QModelIndex &idx) const override
    {
        // in expanded state bottom contains 'Clone', 'Rename', etc links, where the tool tip
        // would be confusing
        const bool expanded = m_expandedSessions.contains(idx.data(Qt::DisplayRole).toString());
        return expanded ? itemRect.adjusted(0, 0, 0, -LINK_HEIGHT) : itemRect;
    }
    int shortcutRole() const override { return SessionModel::ShortcutRole; }

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const final
    {
        static const QPixmap sessionIcon = pixmap("session", Theme::Welcome_ForegroundSecondaryColor);

        const QRect rc = option.rect;
        const QString sessionName = idx.data(Qt::DisplayRole).toString();

        const QPoint mousePos = option.widget->mapFromGlobal(QCursor::pos());
        //const bool hovered = option.state & QStyle::State_MouseOver;
        const bool hovered = option.rect.contains(mousePos);
        const bool expanded = m_expandedSessions.contains(sessionName);
        if (hovered)
            painter->fillRect(expanded ? rc : rc.adjusted(0, 0, -24, 0), hoverColor);

        const int x = rc.x();
        const int x1 = x + 36;
        const int y = rc.y();
        const int firstBase = y + 18;

        painter->drawPixmap(x + 11, y + 6, sessionIcon);

        if (hovered && !expanded) {
            const QRect arrowRect = rc.adjusted(rc.width() - 24, 0, 0, 0);
            if (arrowRect.contains(mousePos))
                painter->fillRect(arrowRect, hoverColor);
        }

        if (hovered || expanded) {
            static const QPixmap arrowUp = pixmap("expandarrow",Theme::Welcome_ForegroundSecondaryColor);
            static const QPixmap arrowDown = QPixmap::fromImage(arrowUp.toImage().mirrored(false, true));
            painter->drawPixmap(rc.right() - 20, y + 7, expanded ? arrowDown : arrowUp);
        }

        if (idx.row() < 9) {
            painter->setPen(foregroundColor2);
            painter->setFont(sizedFont(10, option.widget));
            painter->drawText(x + 3, firstBase, QString::number(idx.row() + 1));
        }

        const bool isLastSession = idx.data(SessionModel::LastSessionRole).toBool();
        const bool isActiveSession = idx.data(SessionModel::ActiveSessionRole).toBool();
        const bool isDefaultVirgin = SessionManager::isDefaultVirgin();

        QString fullSessionName = sessionName;
        if (isLastSession && isDefaultVirgin)
            fullSessionName = ProjectWelcomePage::tr("%1 (last session)").arg(fullSessionName);
        if (isActiveSession && !isDefaultVirgin)
            fullSessionName = ProjectWelcomePage::tr("%1 (current session)").arg(fullSessionName);

        const QRect switchRect = QRect(x, y, rc.width() - 24, SESSION_LINE_HEIGHT);
        const bool switchActive = switchRect.contains(mousePos);
        painter->setPen(linkColor);
        painter->setFont(sizedFont(13, option.widget, switchActive));
        painter->drawText(x1, firstBase, fullSessionName);
        if (switchActive)
            m_activeSwitchToRect = switchRect;

        if (expanded) {
            painter->setPen(textColor);
            painter->setFont(sizedFont(12, option.widget));
            const QStringList projects = SessionManager::projectsForSessionName(sessionName);
            int yy = firstBase + 25;
            QFontMetrics fm(option.widget->font());
            for (const QString &project : projects) {
                // Project name.
                QFileInfo fi(project);
                QString completeBase = fi.completeBaseName();
                painter->setPen(textColor);
                painter->drawText(x1, yy, completeBase);
                yy += 18;

                // Project path.
                QString pathWithTilde = Utils::withTildeHomePath(QDir::toNativeSeparators(project));
                painter->setPen(foregroundColor1);
                painter->drawText(x1, yy, fm.elidedText(pathWithTilde, Qt::ElideMiddle, rc.width() - 40));
                yy += 22;
            }

            yy += 5;
            int xx = x1;
            const QStringList actions = {
                ProjectWelcomePage::tr("Clone"),
                ProjectWelcomePage::tr("Rename"),
                ProjectWelcomePage::tr("Delete")
            };
            for (int i = 0; i < 3; ++i) {
                const QString &action = actions.at(i);
                const int ww = fm.width(action);
                const QRect actionRect(xx, yy - 10, ww, 15);
                const bool isActive = actionRect.contains(mousePos);
                painter->setPen(linkColor);
                painter->setFont(sizedFont(12, option.widget, isActive));
                painter->drawText(xx, yy, action);
                if (i < 2) {
                    xx += ww + 14;
                    int pp = xx - 7;
                    painter->setPen(textColor);
                    painter->drawLine(pp, yy - 10, pp, yy);
                }
                if (isActive)
                    m_activeActionRects[i] = actionRect;
            }
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &idx) const final
    {
        int h = SESSION_LINE_HEIGHT;
        QString sessionName = idx.data(Qt::DisplayRole).toString();
        if (m_expandedSessions.contains(sessionName)) {
            QStringList projects = SessionManager::projectsForSessionName(sessionName);
            h += projects.size() * 40 + LINK_HEIGHT;
        }
        return QSize(380, h);
    }

    bool editorEvent(QEvent *ev, QAbstractItemModel *model,
        const QStyleOptionViewItem &option, const QModelIndex &idx) final
    {
        if (ev->type() == QEvent::MouseButtonRelease) {
            const QPoint pos = static_cast<QMouseEvent *>(ev)->pos();
            const QRect rc(option.rect.right() - 24, option.rect.top(), 24, SESSION_LINE_HEIGHT);
            const QString sessionName = idx.data(Qt::DisplayRole).toString();
            if (rc.contains(pos)) {
                // The expand/collapse "button".
                if (m_expandedSessions.contains(sessionName))
                    m_expandedSessions.removeOne(sessionName);
                else
                    m_expandedSessions.append(sessionName);
                model->layoutChanged({QPersistentModelIndex(idx)});
                return false;
            }
            // One of the action links?
            const auto sessionModel = qobject_cast<SessionModel *>(model);
            QTC_ASSERT(sessionModel, return false);
            if (m_activeSwitchToRect.contains(pos))
                sessionModel->switchToSession(sessionName);
            else if (m_activeActionRects[0].contains(pos))
                sessionModel->cloneSession(sessionName);
            else if (m_activeActionRects[1].contains(pos))
                sessionModel->renameSession(sessionName);
            else if (m_activeActionRects[2].contains(pos))
                sessionModel->deleteSession(sessionName);
            return true;
        }
        if (ev->type() == QEvent::MouseMove) {
            model->layoutChanged({QPersistentModelIndex(idx)}); // Somewhat brutish.
            //update(option.rect);
            return true;
        }
        return false;
    }

private:
    const QColor hoverColor = themeColor(Theme::Welcome_HoverColor);
    const QColor textColor = themeColor(Theme::Welcome_TextColor);
    const QColor linkColor = themeColor(Theme::Welcome_LinkColor);
    const QColor backgroundColor = themeColor(Theme::Welcome_BackgroundColor);
    const QColor foregroundColor1 = themeColor(Theme::Welcome_ForegroundPrimaryColor); // light-ish.
    const QColor foregroundColor2 = themeColor(Theme::Welcome_ForegroundSecondaryColor); // blacker.

    QStringList m_expandedSessions;

    mutable QRect m_activeSwitchToRect;
    mutable QRect m_activeActionRects[3];
};

class ProjectDelegate : public BaseDelegate
{
    QString entryType() override
    {
        return ProjectWelcomePage::tr("project", "Appears in \"Open project <name>\"");
    }
    int shortcutRole() const override { return ProjectModel::ShortcutRole; }

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const final
    {
        QRect rc = option.rect;

        const bool hovered = option.widget->isActiveWindow() && option.state & QStyle::State_MouseOver;
        if (hovered)
            painter->fillRect(rc, themeColor(Theme::Welcome_HoverColor));

        const int x = rc.x();
        const int y = rc.y();
        const int firstBase = y + 18;
        const int secondBase = firstBase + 19;

        static const QPixmap projectIcon = pixmap("project", Theme::Welcome_ForegroundSecondaryColor);
        painter->drawPixmap(x + 11, y + 6, projectIcon);

        QString projectName = idx.data(Qt::DisplayRole).toString();
        QString projectPath = idx.data(ProjectModel::FilePathRole).toString();

        painter->setPen(themeColor(Theme::Welcome_ForegroundSecondaryColor));
        painter->setFont(sizedFont(10, option.widget));

        if (idx.row() < 9)
            painter->drawText(x + 3, firstBase, QString::number(idx.row() + 1));

        painter->setPen(themeColor(Theme::Welcome_LinkColor));
        painter->setFont(sizedFont(13, option.widget, hovered));
        painter->drawText(x + 36, firstBase, projectName);

        painter->setPen(themeColor(Theme::Welcome_ForegroundPrimaryColor));
        painter->setFont(sizedFont(13, option.widget));
        QString pathWithTilde = Utils::withTildeHomePath(QDir::toNativeSeparators(projectPath));
        painter->drawText(x + 36, secondBase, pathWithTilde);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &idx) const final
    {
        QString projectName = idx.data(Qt::DisplayRole).toString();
        QString projectPath = idx.data(ProjectModel::FilePathRole).toString();
        QFontMetrics fm(sizedFont(13, option.widget));
        int width = std::max(fm.width(projectName), fm.width(projectPath)) + 36;
        return QSize(width, 48);
    }

    bool editorEvent(QEvent *ev, QAbstractItemModel *,
        const QStyleOptionViewItem &, const QModelIndex &idx) final
    {
        if (ev->type() == QEvent::MouseButtonRelease) {
            QString projectFile = idx.data(ProjectModel::FilePathRole).toString();
            ProjectExplorerPlugin::openProjectWelcomePage(projectFile);
            return true;
        }
        return false;
    }
};

class TreeView : public QTreeView
{
public:
    TreeView(QWidget *parent)
        : QTreeView(parent)
    {
        header()->hide();
        setMouseTracking(true); // To enable hover.
        setIndentation(0);
        setSelectionMode(QAbstractItemView::NoSelection);
        setFrameShape(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setFocusPolicy(Qt::NoFocus);

        QPalette pal;  // Needed for classic theme (only).
        pal.setColor(QPalette::Base, themeColor(Theme::Welcome_BackgroundColor));
        viewport()->setPalette(pal);
    }

    void leaveEvent(QEvent *) final
    {
        QHoverEvent hev(QEvent::HoverLeave, QPointF(), QPointF());
        viewportEvent(&hev); // Seemingly needed to kill the hover paint.
    }
};

class SessionsPage : public QWidget
{
public:
    SessionsPage(ProjectWelcomePage *projectWelcomePage)
    {
        // FIXME: Remove once facilitateQml() is gone.
        if (!projectWelcomePage->m_sessionModel)
            projectWelcomePage->m_sessionModel = new SessionModel(this);
        if (!projectWelcomePage->m_projectModel)
            projectWelcomePage->m_projectModel = new ProjectModel(this);

        auto newButton = new WelcomePageButton(this);
        newButton->setText(ProjectWelcomePage::tr("New Project"));
        newButton->setIcon(pixmap("new", Theme::Welcome_ForegroundSecondaryColor));
        newButton->setOnClicked([] { ProjectExplorerPlugin::openNewProjectDialog(); });

        auto openButton = new WelcomePageButton(this);
        openButton->setText(ProjectWelcomePage::tr("Open Project"));
        openButton->setIcon(pixmap("open", Theme::Welcome_ForegroundSecondaryColor));
        openButton->setOnClicked([] { ProjectExplorerPlugin::openOpenProjectDialog(); });

        auto sessionsLabel = new QLabel(this);
        sessionsLabel->setFont(sizedFont(16, this));
        sessionsLabel->setText(ProjectWelcomePage::tr("Sessions"));

        auto recentProjectsLabel = new QLabel(this);
        recentProjectsLabel->setFont(sizedFont(16, this));
        recentProjectsLabel->setText(ProjectWelcomePage::tr("Recent Projects"));

        auto sessionsList = new TreeView(this);
        sessionsList->setModel(projectWelcomePage->m_sessionModel);
        sessionsList->header()->setSectionHidden(1, true); // The "last modified" column.
        sessionsList->setItemDelegate(&m_sessionDelegate);
        sessionsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto projectsList = new TreeView(this);
        projectsList->setUniformRowHeights(true);
        projectsList->setModel(projectWelcomePage->m_projectModel);
        projectsList->setItemDelegate(&m_projectDelegate);
        projectsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto hbox11 = new QHBoxLayout;
        hbox11->setContentsMargins(0, 0, 0, 0);
        hbox11->addWidget(newButton);
        hbox11->addStretch(1);

        auto hbox21 = new QHBoxLayout;
        hbox21->setContentsMargins(0, 0, 0, 0);
        hbox21->addWidget(openButton);
        hbox21->addStretch(1);

        auto vbox1 = new QVBoxLayout;
        vbox1->setContentsMargins(0, 0, 0, 0);
        vbox1->addStrut(200);
        vbox1->addItem(hbox11);
        vbox1->addSpacing(16);
        vbox1->addWidget(sessionsLabel);
        vbox1->addSpacing(21);
        vbox1->addWidget(sessionsList);

        auto vbox2 = new QVBoxLayout;
        vbox2->setContentsMargins(0, 0, 0, 0);
        vbox2->addItem(hbox21);
        vbox2->addSpacing(16);
        vbox2->addWidget(recentProjectsLabel);
        vbox2->addSpacing(21);
        vbox2->addWidget(projectsList);

        auto hbox = new QHBoxLayout(this);
        hbox->setContentsMargins(30, 27, 0, 27);
        hbox->addItem(vbox1);
        hbox->addSpacing(16);
        hbox->addItem(vbox2);
        hbox->setStretchFactor(vbox2, 2);
    }

    SessionDelegate m_sessionDelegate;
    ProjectDelegate m_projectDelegate;
};

QWidget *ProjectWelcomePage::createWidget() const
{
    return new SessionsPage(const_cast<ProjectWelcomePage *>(this));
}

} // namespace Internal
} // namespace ProjectExplorer
