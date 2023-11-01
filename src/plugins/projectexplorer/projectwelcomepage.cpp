// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectwelcomepage.h"

#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/session.h>
#include <coreplugin/sessionmodel.h>
#include <coreplugin/welcomepagehelper.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QAbstractItemDelegate>
#include <QAction>
#include <QBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>

using namespace Core;
using namespace Core::WelcomePageHelpers;
using namespace Utils;

const int LINK_HEIGHT = 35;
const int TEXT_OFFSET_HORIZONTAL = 36;
const int SESSION_LINE_HEIGHT = 28;
const int SESSION_ARROW_RECT_WIDTH = 24;
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
    return m_projects.count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if (m_projects.count() <= index.row())
        return {};
    RecentProjectsEntry data = m_projects.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.second;
    case Qt::ToolTipRole:
    case FilePathRole:
        return data.first.toVariant();
    case PrettyFilePathRole:
        return data.first.withTildeHomePath(); // FIXME: FilePath::displayName() ?
    case ShortcutRole: {
        const Id projectBase = PROJECT_BASE_ID;
        if (Command *cmd = ActionManager::command(projectBase.withSuffix(index.row() + 1)))
            return cmd->keySequence().toString(QKeySequence::NativeText);
        return {};
    }
    default:
        return {};
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
    m_projects = ProjectExplorerPlugin::recentProjects();
    endResetModel();
}

///////////////////

ProjectWelcomePage::ProjectWelcomePage()
{
}

Utils::Id ProjectWelcomePage::id() const
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
    const QVariant projectFile = m_projectModel->data(m_projectModel->index(index, 0),
                                                      ProjectModel::FilePathRole);
    ProjectExplorerPlugin::openProjectWelcomePage(FilePath::fromVariant(projectFile));
}

void ProjectWelcomePage::createActions()
{
    static bool actionsRegistered = false;

    if (actionsRegistered)
        return;

    actionsRegistered = true;

    const int actionsCount = 9;
    Context welcomeContext(Core::Constants::C_WELCOME_MODE);

    const Id projectBase = PROJECT_BASE_ID;
    const Id sessionBase = SESSION_BASE_ID;

    for (int i = 1; i <= actionsCount; ++i) {
        auto act = new QAction(Tr::tr("Open Session #%1").arg(i), this);
        Command *cmd = ActionManager::registerAction(act, sessionBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence((useMacShortcuts ? Tr::tr("Ctrl+Meta+%1") : Tr::tr("Ctrl+Alt+%1")).arg(i)));
        connect(act, &QAction::triggered, this, [this, i] {
            if (i <= m_sessionModel->rowCount())
                openSessionAt(i - 1);
        });

        act = new QAction(Tr::tr("Open Recent Project #%1").arg(i), this);
        cmd = ActionManager::registerAction(act, projectBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+%1").arg(i)));
        connect(act, &QAction::triggered, this, [this, i] {
            if (i <= m_projectModel->rowCount(QModelIndex()))
                openProjectAt(i - 1);
        });
    }
}

///////////////////

static QColor themeColor(Theme::Color role)
{
    return Utils::creatorTheme()->color(role);
}

static QFont sizedFont(int size, const QWidget *widget,
                       bool underline = false)
{
    QFont f = widget->font();
    f.setPixelSize(size);
    f.setUnderline(underline);
    return f;
}

static QPixmap pixmap(const QString &id, const Theme::Color &color)
{
    const QString fileName = QString(":/welcome/images/%1.png").arg(id);
    return Icon({{FilePath::fromString(fileName), color}}, Icon::Tint).pixmap();
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
            tooltipText = Tr::tr("Open %1 \"%2\"").arg(type, name);
        else
            tooltipText = Tr::tr("Open %1 \"%2\" (%3)").arg(type, name, shortcut);

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
        return Tr::tr("session", "Appears in \"Open session <name>\"");
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
        painter->fillRect(rc, themeColor(Theme::Welcome_BackgroundSecondaryColor));
        painter->fillRect(rc.adjusted(0, 0, 0, -ItemGap),
                          hovered ? hoverColor : backgroundPrimaryColor);

        const int x = rc.x();
        const int x1 = x + TEXT_OFFSET_HORIZONTAL;
        const int y = rc.y();
        const int firstBase = y + 18;

        painter->drawPixmap(x + 11, y + 6, sessionIcon);

        if (hovered && !expanded) {
            const QRect arrowRect = rc.adjusted(rc.width() - SESSION_ARROW_RECT_WIDTH, 0, 0, 0);
            const bool arrowRectHovered = arrowRect.contains(mousePos);
            painter->fillRect(arrowRect.adjusted(0, 0, 0, -ItemGap),
                              arrowRectHovered ? hoverColor : backgroundPrimaryColor);
        }

        if (hovered || expanded) {
            static const QPixmap arrowUp = pixmap("expandarrow",Theme::Welcome_ForegroundSecondaryColor);
            static const QPixmap arrowDown = QPixmap::fromImage(arrowUp.toImage().mirrored(false, true));
            painter->drawPixmap(rc.right() - 19, y + 6, expanded ? arrowDown : arrowUp);
        }

        if (idx.row() < 9) {
            painter->setPen(foregroundSecondaryColor);
            painter->setFont(sizedFont(10, option.widget));
            painter->drawText(x + 3, firstBase, QString::number(idx.row() + 1));
        }

        const bool isLastSession = idx.data(SessionModel::LastSessionRole).toBool();
        const bool isActiveSession = idx.data(SessionModel::ActiveSessionRole).toBool();
        const bool isDefaultVirgin = SessionManager::isDefaultVirgin();

        QString fullSessionName = sessionName;
        if (isLastSession && isDefaultVirgin)
            fullSessionName = Tr::tr("%1 (last session)").arg(fullSessionName);
        if (isActiveSession && !isDefaultVirgin)
            fullSessionName = Tr::tr("%1 (current session)").arg(fullSessionName);

        const QRect switchRect = QRect(x, y, rc.width() - SESSION_ARROW_RECT_WIDTH, SESSION_LINE_HEIGHT);
        const bool switchActive = switchRect.contains(mousePos);
        const int textSpace = rc.width() - TEXT_OFFSET_HORIZONTAL - 6;
        const int sessionNameTextSpace =
                textSpace -(hovered || expanded ? SESSION_ARROW_RECT_WIDTH : 0);
        painter->setPen(linkColor);
        painter->setFont(sizedFont(13, option.widget, switchActive));
        const QString fullSessionNameElided = painter->fontMetrics().elidedText(
                    fullSessionName, Qt::ElideRight, sessionNameTextSpace);
        painter->drawText(x1, firstBase, fullSessionNameElided);
        if (switchActive)
            m_activeSwitchToRect = switchRect;

        if (expanded) {
            painter->setPen(textColor);
            painter->setFont(sizedFont(12, option.widget));
            const FilePaths projects = ProjectManager::projectsForSessionName(sessionName);
            int yy = firstBase + SESSION_LINE_HEIGHT - 3;
            QFontMetrics fm(option.widget->font());
            for (const FilePath &projectPath : projects) {
                // Project name.
                QString completeBase = projectPath.completeBaseName();
                painter->setPen(textColor);
                painter->drawText(x1, yy, fm.elidedText(completeBase, Qt::ElideMiddle, textSpace));
                yy += 18;

                // Project path.
                const QString displayPath =
                    projectPath.osType() == OsTypeWindows ? projectPath.displayName()
                                                          : projectPath.withTildeHomePath();
                painter->setPen(foregroundPrimaryColor);
                painter->drawText(x1, yy, fm.elidedText(displayPath, Qt::ElideMiddle, textSpace));
                yy += 22;
            }

            yy += 3;
            int xx = x1;
            const QStringList actions = {
                Tr::tr("Clone"),
                Tr::tr("Rename"),
                Tr::tr("Delete")
            };
            for (int i = 0; i < 3; ++i) {
                const QString &action = actions.at(i);
                const int ww = fm.horizontalAdvance(action);
                const int spacing = 7; // Between action link and separator line
                const QRect actionRect =
                        QRect(xx, yy - 10, ww, 15).adjusted(-spacing, -spacing, spacing, spacing);
                const bool isForcedDisabled = (i != 0 && sessionName == "default");
                const bool isActive = actionRect.contains(mousePos) && !isForcedDisabled;
                painter->setPen(isForcedDisabled ? disabledLinkColor : linkColor);
                painter->setFont(sizedFont(12, option.widget, isActive));
                painter->drawText(xx, yy, action);
                if (i < 2) {
                    xx += ww + 2 * spacing;
                    int pp = xx - spacing;
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
            const FilePaths projects = ProjectManager::projectsForSessionName(sessionName);
            h += projects.size() * 40 + LINK_HEIGHT - 6;
        }
        return QSize(380, h + ItemGap);
    }

    bool editorEvent(QEvent *ev, QAbstractItemModel *model,
        const QStyleOptionViewItem &option, const QModelIndex &idx) final
    {
        if (ev->type() == QEvent::MouseButtonRelease) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(ev);
            const Qt::MouseButtons button = mouseEvent->button();
            const QPoint pos = static_cast<QMouseEvent *>(ev)->pos();
            const QRect rc(option.rect.right() - SESSION_ARROW_RECT_WIDTH, option.rect.top(),
                           SESSION_ARROW_RECT_WIDTH, SESSION_LINE_HEIGHT);
            const QString sessionName = idx.data(Qt::DisplayRole).toString();
            if (rc.contains(pos) || button == Qt::RightButton) {
                // The expand/collapse "button".
                if (m_expandedSessions.contains(sessionName))
                    m_expandedSessions.removeOne(sessionName);
                else
                    m_expandedSessions.append(sessionName);
                emit model->layoutChanged({QPersistentModelIndex(idx)});
                return true;
            }
            if (button == Qt::LeftButton) {
                // One of the action links?
                const auto sessionModel = qobject_cast<SessionModel *>(model);
                QTC_ASSERT(sessionModel, return false);
                if (m_activeSwitchToRect.contains(pos))
                    sessionModel->switchToSession(sessionName);
                else if (m_activeActionRects[0].contains(pos))
                    sessionModel->cloneSession(ICore::dialogParent(), sessionName);
                else if (m_activeActionRects[1].contains(pos))
                    sessionModel->renameSession(ICore::dialogParent(), sessionName);
                else if (m_activeActionRects[2].contains(pos))
                    sessionModel->deleteSessions(QStringList(sessionName));
                return true;
            }
        }
        if (ev->type() == QEvent::MouseMove) {
            emit model->layoutChanged({QPersistentModelIndex(idx)}); // Somewhat brutish.
            //update(option.rect);
            return false;
        }
        return false;
    }

private:
    const QColor hoverColor = themeColor(Theme::Welcome_HoverColor);
    const QColor textColor = themeColor(Theme::Welcome_TextColor);
    const QColor linkColor = themeColor(Theme::Welcome_LinkColor);
    const QColor disabledLinkColor = themeColor(Theme::Welcome_DisabledLinkColor);
    const QColor backgroundPrimaryColor = themeColor(Theme::Welcome_BackgroundPrimaryColor);
    const QColor foregroundPrimaryColor = themeColor(Theme::Welcome_ForegroundPrimaryColor);
    const QColor foregroundSecondaryColor = themeColor(Theme::Welcome_ForegroundSecondaryColor);

    QStringList m_expandedSessions;

    mutable QRect m_activeSwitchToRect;
    mutable QRect m_activeActionRects[3];
};

class ProjectDelegate : public BaseDelegate
{
    QString entryType() override
    {
        return Tr::tr("project", "Appears in \"Open project <name>\"");
    }
    int shortcutRole() const override { return ProjectModel::ShortcutRole; }

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const final
    {
        QRect rc = option.rect;

        const bool hovered = option.widget->isActiveWindow() && option.state & QStyle::State_MouseOver;
        const QRect bgRect = rc.adjusted(0, 0, -ItemGap, -ItemGap);
        painter->fillRect(rc, themeColor(Theme::Welcome_BackgroundSecondaryColor));
        painter->fillRect(bgRect, themeColor(hovered ? Theme::Welcome_HoverColor
                                                     : Theme::Welcome_BackgroundPrimaryColor));

        const int x = rc.x();
        const int y = rc.y();
        const int firstBase = y + 18;
        const int secondBase = firstBase + 19;

        static const QPixmap projectIcon =
                pixmap("project", Theme::Welcome_ForegroundSecondaryColor);
        painter->drawPixmap(x + 11, y + 6, projectIcon);

        QString projectName = idx.data(Qt::DisplayRole).toString();
        FilePath projectPath = FilePath::fromVariant(idx.data(ProjectModel::FilePathRole));

        painter->setPen(themeColor(Theme::Welcome_ForegroundSecondaryColor));
        painter->setFont(sizedFont(10, option.widget));

        if (idx.row() < 9)
            painter->drawText(x + 3, firstBase, QString::number(idx.row() + 1));

        const int textSpace = rc.width() - TEXT_OFFSET_HORIZONTAL - ItemGap - 6;

        painter->setPen(themeColor(Theme::Welcome_LinkColor));
        painter->setFont(sizedFont(13, option.widget, hovered));
        const QString projectNameElided =
                painter->fontMetrics().elidedText(projectName, Qt::ElideRight, textSpace);
        painter->drawText(x + TEXT_OFFSET_HORIZONTAL, firstBase, projectNameElided);

        painter->setPen(themeColor(Theme::Welcome_ForegroundPrimaryColor));
        painter->setFont(sizedFont(13, option.widget));
        const QString displayPath =
            projectPath.osType() == OsTypeWindows ? projectPath.displayName()
                                                  : projectPath.withTildeHomePath();
        const QString displayPathElided =
            painter->fontMetrics().elidedText(displayPath, Qt::ElideMiddle, textSpace);
        painter->drawText(x + TEXT_OFFSET_HORIZONTAL, secondBase, displayPathElided);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &idx) const final
    {
        QString projectName = idx.data(Qt::DisplayRole).toString();
        QString projectPath = idx.data(ProjectModel::FilePathRole).toString();
        QFontMetrics fm(sizedFont(13, option.widget));
        int width = std::max(fm.horizontalAdvance(projectName),
                             fm.horizontalAdvance(projectPath)) + TEXT_OFFSET_HORIZONTAL;
        return QSize(width, 47 + ItemGap);
    }

    bool editorEvent(QEvent *ev, QAbstractItemModel *model,
        const QStyleOptionViewItem &, const QModelIndex &idx) final
    {
        if (ev->type() == QEvent::MouseButtonRelease) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(ev);
            const Qt::MouseButtons button = mouseEvent->button();
            if (button == Qt::LeftButton) {
                const QVariant projectFile = idx.data(ProjectModel::FilePathRole);
                ProjectExplorerPlugin::openProjectWelcomePage(FilePath::fromVariant(projectFile));
                return true;
            }
            if (button == Qt::RightButton) {
                QMenu contextMenu;
                QAction *action = new QAction(Tr::tr("Remove Project from Recent Projects"));
                const auto projectModel = qobject_cast<ProjectModel *>(model);
                contextMenu.addAction(action);
                connect(action, &QAction::triggered, this, [idx, projectModel] {
                    const QVariant projectFile = idx.data(ProjectModel::FilePathRole);
                    ProjectExplorerPlugin::removeFromRecentProjects(FilePath::fromVariant(projectFile));
                    projectModel->resetProjects();
                });
                contextMenu.addSeparator();
                action = new QAction(Tr::tr("Clear Recent Project List"));
                connect(action, &QAction::triggered, this, [projectModel] {
                    ProjectExplorerPlugin::clearRecentProjects();
                    projectModel->resetProjects();
                });
                contextMenu.addAction(action);
                contextMenu.exec(mouseEvent->globalPosition().toPoint());
                return true;
            }
        }
        return false;
    }
};

class TreeView : public QTreeView
{
public:
    TreeView(QWidget *parent, const QString &name)
        : QTreeView(parent)
    {
        setObjectName(name);
        header()->hide();
        setMouseTracking(true); // To enable hover.
        setIndentation(0);
        setSelectionMode(QAbstractItemView::NoSelection);
        setFrameShape(QFrame::NoFrame);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        setFocusPolicy(Qt::NoFocus);

        QPalette pal;
        pal.setColor(QPalette::Base, themeColor(Theme::Welcome_BackgroundSecondaryColor));
        viewport()->setPalette(pal);
    }
};

class SessionsPage : public QWidget
{
public:
    explicit SessionsPage(ProjectWelcomePage *projectWelcomePage)
    {
        // FIXME: Remove once facilitateQml() is gone.
        if (!projectWelcomePage->m_sessionModel)
            projectWelcomePage->m_sessionModel = new SessionModel(this);
        if (!projectWelcomePage->m_projectModel)
            projectWelcomePage->m_projectModel = new ProjectModel(this);

        auto manageSessionsButton = new WelcomePageButton(this);
        manageSessionsButton->setText(Tr::tr("Manage..."));
        manageSessionsButton->setWithAccentColor(true);
        manageSessionsButton->setOnClicked([] { SessionManager::showSessionManager(); });

        auto sessionsLabel = new QLabel(this);
        sessionsLabel->setFont(brandFont());
        sessionsLabel->setText(Tr::tr("Sessions"));

        auto recentProjectsLabel = new QLabel(this);
        recentProjectsLabel->setFont(brandFont());
        recentProjectsLabel->setText(Tr::tr("Projects"));

        auto sessionsList = new TreeView(this, "Sessions");
        sessionsList->setModel(projectWelcomePage->m_sessionModel);
        sessionsList->header()->setSectionHidden(1, true); // The "last modified" column.
        sessionsList->setItemDelegate(&m_sessionDelegate);
        sessionsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto projectsList = new TreeView(this, "Recent Projects");
        projectsList->setUniformRowHeights(true);
        projectsList->setModel(projectWelcomePage->m_projectModel);
        projectsList->setItemDelegate(&m_projectDelegate);
        projectsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        auto sessionHeader = panelBar(this);
        auto hbox11 = new QHBoxLayout(sessionHeader);
        hbox11->setContentsMargins(12, 0, 0, 0);
        hbox11->addWidget(sessionsLabel);
        hbox11->addStretch(1);
        hbox11->addWidget(manageSessionsButton);

        auto projectsHeader = panelBar(this);
        auto hbox21 = new QHBoxLayout(projectsHeader);
        hbox21->setContentsMargins(hbox11->contentsMargins());
        hbox21->addWidget(recentProjectsLabel);

        auto grid = new QGridLayout(this);
        grid->setContentsMargins(0, 0, 0, ItemGap);
        grid->setHorizontalSpacing(0);
        grid->setVerticalSpacing(ItemGap);
        grid->addWidget(panelBar(this), 0, 0);
        grid->addWidget(sessionHeader, 0, 1);
        grid->addWidget(sessionsList, 1, 1);
        grid->addWidget(panelBar(this), 0, 2);
        grid->setColumnStretch(1, 9);
        grid->setColumnMinimumWidth(1, 200);
        grid->addWidget(projectsHeader, 0, 3);
        grid->addWidget(projectsList, 1, 3);
        grid->setColumnStretch(3, 20);
    }

    SessionDelegate m_sessionDelegate;
    ProjectDelegate m_projectDelegate;
};

QWidget *ProjectWelcomePage::createWidget() const
{
    auto that = const_cast<ProjectWelcomePage *>(this);
    QWidget *widget = new SessionsPage(that);
    that->createActions();

    return widget;
}

} // namespace Internal
} // namespace ProjectExplorer
