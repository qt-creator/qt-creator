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
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/session.h>
#include <coreplugin/sessionmodel.h>
#include <coreplugin/welcomepagehelper.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAbstractItemDelegate>
#include <QAction>
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
using namespace Utils::StyleHelper::SpacingTokens;

const char PROJECT_BASE_ID[] = "Welcome.OpenRecentProject";
static const qsizetype kMaxPathsDisplay = 5;

namespace ProjectExplorer {
namespace Internal {

constexpr TextFormat projectNameTF {Theme::Token_Text_Accent, StyleHelper::UiElementH5};
constexpr TextFormat projectPathTF {Theme::Token_Text_Muted, StyleHelper::UiElementH6};
constexpr TextFormat sessionNameTF = projectNameTF;
constexpr TextFormat sessionProjectNameTF {Theme::Token_Text_Default, projectNameTF.uiElement};
constexpr TextFormat shortcutNumberTF {Theme::Token_Text_Default,
                                      StyleHelper::UiElementCaptionStrong,
                                      Qt::AlignCenter | Qt::TextDontClip};
constexpr TextFormat actionTF {Theme::Token_Text_Default, StyleHelper::UiElementIconActive,
                              Qt::AlignCenter | Qt::TextDontClip};
constexpr TextFormat actionDisabledTF {Theme::Token_Text_Subtle, actionTF.uiElement,
                                      actionTF.drawTextFlags};
constexpr int shortcutNumberWidth = 6;
constexpr int actionSepWidth = 1;
constexpr int sessionScrollBarGap = HPaddingXs;

static int s(const int metric)
{
    constexpr int shrinkWhenAbove = 150; // Above this session count, increasingly reduce scale
    constexpr qreal maxScale = 1.0; // Spacings as defined by design
    constexpr qreal minScale = 0.2; // Maximum "condensed" layout

    const int sessionsCount = SessionManager::sessionsCount();
    const qreal scaling = sessionsCount < shrinkWhenAbove
                              ? maxScale
                              : qMax(minScale,
                                     maxScale - (sessionsCount - shrinkWhenAbove) * 0.065);
    return int(qMax(1.0, scaling * metric));
}

static int itemSpacing()
{
    return qMax(int(s(VGapL)), VGapS);
}

static bool withIcon()
{
    return s(100) > 60; // Hide icons if spacings are scaled to below 60%
}

static FilePaths pathsForSession(const QString &session, QString *title = nullptr)
{
    const FilePaths projects = ProjectManager::projectsForSessionName(session);
    if (projects.size()) {
        if (title) {
            //: title in expanded session items in welcome mode
            *title = Tr::tr("Projects");
        }
        return projects;
    }
    const FilePaths allPaths = SessionManager::openFilesForSessionName(
        session, kMaxPathsDisplay + 1 /* +1 so we know if there are more */);
    if (title) {
        //: title in expanded session items in welcome mode
        *title = Tr::tr("Files");
    }
    return allPaths;
}

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::recentProjectsChanged,
            this, &ProjectModel::resetProjects);
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return int(m_projects.count());
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if (m_projects.count() <= index.row())
        return {};
    RecentProjectsEntry data = m_projects.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.displayName;
    case Qt::ToolTipRole:
    case FilePathRole:
        return data.filePath.toVariant();
    case PrettyFilePathRole:
        return data.filePath.withTildeHomePath(); // FIXME: FilePath::displayName() ?
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

static QPixmap pixmap(const QString &id, const Theme::Color color)
{
    const QString fileName = QString(":/welcome/images/%1.png").arg(id);
    return Icon({{FilePath::fromString(fileName), color}}, Icon::Tint).pixmap();
}

static void drawBackgroundRect(QPainter *painter, const QRectF &rect, bool hovered)
{
    const QColor fill(creatorColor(hovered ? cardHoverBackground : cardDefaultBackground));
    const QPen pen(creatorColor(hovered ? cardHoverStroke : cardDefaultStroke));

    const qreal rounding = s(defaultCardBackgroundRounding * 1000) / 1000.0;
    const qreal saneRounding = rounding <= 2 ? 0 : rounding;
    WelcomePageHelpers::drawCardBackground(painter, rect, fill, pen, saneRounding);
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
    bool expanded(const QModelIndex &idx) const
    {
        return m_expandedSessions.contains(idx.data(Qt::DisplayRole).toString());
    }

    QString entryType() override
    {
        return Tr::tr("session", "Appears in \"Open session <name>\"");
    }
    QRect toolTipArea(const QRect &itemRect, const QModelIndex &idx) const override
    {
        // in expanded state bottom contains 'Clone', 'Rename', etc links, where the tool tip
        // would be confusing
        return expanded(idx) ? itemRect.adjusted(0, 0, 0, -actionButtonHeight()) : itemRect;
    }

    int shortcutRole() const override
    {
        return SessionModel::ShortcutRole;
    }

    static int actionButtonHeight()
    {
        return s(VPaddingXxs) + actionTF.lineHeight() + s(VPaddingXxs);
    }

    static const QPixmap &icon()
    {
        static const QPixmap icon = pixmap("session", Theme::Token_Text_Muted);
        return icon;
    }

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &idx) const final
    {
        //                                       visible on withIcon()        Gap + arrow visible on hover   Extra margin right of project item
        //                                                |                                 |                               |
        //                                     +----------+----------+             +--------+-------+            +----------+----------+
        //                                     |                     |             |                |            |                     |
        //
        //      +------------+--------+--------+------------+--------+-------------+--------+-------+------------+---------------------+  --+
        //      |            |        |        |(VPaddingXs)|        |(VPaddingXs) |        |       |            |                     |    |
        //      |            |        |        +------------+        +-------------+        |       |            |                     |    |
        //      |(HPaddingXs)|<number>|(HGapXs)|   <icon>   |(HGapXs)|<sessionName>|(HGapXs)|<arrow>|            |                     |    +-- Header
        //      |            |(w:6)   |        +------------+        +-------------+        |       |            |                     |    |
        //      |            |        |        |(VPaddingXs)|        |(VPaddingXs) |        |       |            |                     |    |
        //      |------------+--------+--------+------------+--------+-------------+--------+-------+            |                     |  --+
        //      |                                                    |         (VPaddingXxs)        |            |                     |    |
        //      |                                                    +------------------------------+(HPaddingXs)|                     |    |
        //      |                                                    |      <Projects | Files>      |            |                     |    |
        //      |                                                    +------------------------------+            |                     |    |
        //      |                                                    |        (ExPaddingGapS)       |            |(sessionScrollBarGap)|    |
        //      |                                               +--  +------------------------------+            |                     |    |
        //      |                                               |    |            <path>            |            |                     |    |
        //      |                  Per open project or file   --+    +------------------------------+            |                     |    +-- Expansion
        //      |                                               +--  |         (VPaddingXxs)        |            |                     |    |
        //      +----------------------------------------------------+------------------------------+------------+                     |    |
        //      |                                            (VGapXs)                                            |                     |    |
        //      +---------------------------------------+--------------+-----------------------------------------+                     |    |
        // +--  |                           <cloneButton>|<renameButton>|<deleteButton>                          |                     |    |
        // |    +---------------------------------------+--------------+-----------------------------------------+                     |    |
        // |    |                                          (VPaddingXs)                                          |                     |    |
        // |    +------------------------------------------------------------------------------------------------+---------------------+  --+
        // |    |                                                        (VGapL)                                                       |    +-- Gap between session items
        // |    +----------------------------------------------------------------------------------------------------------------------+  --+
        // |
        // \    session action "buttons" and dividers
        // +-----------------------------------------------+--------+---------+--------+
        // |                    (VGapXs)                   |        |         |        |
        // +----------------+-------------+----------------+        |         |        |
        // |(EXSPaddingGapM)|<buttonLabel>|(EXSPaddingGapM)|(HGapXs)|<divider>|(HGapXs)|
        // +----------------+-------------+----------------+        |(w:1)    |        |
        // |                    (VGapXs)                   |        |         |        |
        // +-----------------------------------------------+--------+---------+--------+
        //
        //                                                 |                           |
        //                                                 +-------------+-------------+
        //                                                               |
        //                                                   omitted after last button

        const QPoint mousePos = option.widget->mapFromGlobal(QCursor::pos());
        const bool hovered = option.rect.contains(mousePos);
        const bool expanded = this->expanded(idx);

        const QRect bgR = option.rect.adjusted(0, 0, -sessionScrollBarGap, -itemSpacing());
        const QRect hdR(bgR.topLeft(), QSize(bgR.width(), expanded ? headerHeight()
                                                                   : bgR.height()));

        const QSize iconS = icon().deviceIndependentSize().toSize();
        static const QPixmap arrow = Icon({{FilePath::fromString(":/core/images/expandarrow.png"),
                                            Theme::Token_Text_Muted}}, Icon::Tint).pixmap();
        const QSize arrowS = arrow.deviceIndependentSize().toSize();
        const bool arrowVisible = hovered || expanded;

        const QString sessionName = idx.data(Qt::DisplayRole).toString();

        const int x = bgR.x();
        const int y = bgR.y();

        const int numberX = x + s(HPaddingXs);
        const int iconX = numberX + shortcutNumberWidth + s(HGapXs);
        const int arrowX = bgR.right() - s(HPaddingXs) - arrowS.width();
        const QRect arrowHoverR(arrowX - s(HGapXs) + 1, y,
                                s(HGapXs) + arrowS.width() + s(HPaddingXs), hdR.height());
        const int textX = withIcon() ? iconX + iconS.width() + s(HGapXs) : iconX;

        const int iconY = y + (hdR.height() - iconS.height()) / 2;
        const int arrowY = y + (hdR.height() - arrowS.height()) / 2;

        {
            drawBackgroundRect(painter, bgR, hovered);
        }
        if (idx.row() < 9) {
            painter->setPen(shortcutNumberTF.color());
            painter->setFont(shortcutNumberTF.font());
            const QRect numberR(numberX, y, shortcutNumberWidth, hdR.height());
            const QString numberString = QString::number(idx.row() + 1);
            painter->drawText(numberR, shortcutNumberTF.drawTextFlags, numberString);
        }
        if (withIcon()) {
            painter->drawPixmap(iconX, iconY, icon());
        }
        {
            const bool isLastSession = idx.data(SessionModel::LastSessionRole).toBool();
            const bool isActiveSession = idx.data(SessionModel::ActiveSessionRole).toBool();
            const bool isDefaultVirgin = SessionManager::isDefaultVirgin();

            const int sessionNameWidth = hdR.right()
                                         - (arrowVisible ? arrowHoverR.width(): s(HPaddingXs))
                                         - textX;
            const int sessionNameHeight = sessionNameTF.lineHeight();
            const int sessionNameY = y + (hdR.height() - sessionNameHeight) / 2;
            const QRect sessionNameR(textX, sessionNameY, sessionNameWidth, sessionNameHeight);

            QString fullSessionName = sessionName;
            if (isLastSession && isDefaultVirgin)
                fullSessionName = Tr::tr("%1 (last session)").arg(fullSessionName);
            if (isActiveSession && !isDefaultVirgin)
                fullSessionName = Tr::tr("%1 (current session)").arg(fullSessionName);
            const QRect switchR(x, y, hdR.width() - arrowHoverR.width(), arrowHoverR.height());
            const bool switchActive = switchR.contains(mousePos);
            painter->setPen(sessionNameTF.color());
            painter->setFont(sessionNameTF.font(switchActive));
            const QString fullSessionNameElided = painter->fontMetrics().elidedText(
                fullSessionName, Qt::ElideRight, sessionNameWidth);
            painter->drawText(sessionNameR, sessionNameTF.drawTextFlags,
                              fullSessionNameElided);
            if (switchActive)
                m_activeSwitchToRect = switchR;
        }
        if (arrowVisible) {
            if (arrowHoverR.adjusted(0, 0, 0, expanded ? 0 : s(VGapL)).contains(mousePos)) {
                m_activeExpandRect = arrowHoverR;
            } else {
                painter->save();
                painter->setClipRect(arrowHoverR);
                drawBackgroundRect(painter, bgR, false);
                painter->restore();
            }
            static const QPixmap arrowDown =
                QPixmap::fromImage(arrow.toImage().mirrored(false, true));
            painter->drawPixmap(arrowX, arrowY, expanded ? arrowDown : arrow);
        }

        int yy = hdR.bottom();
        if (expanded) {
            const QFont titleFont = sessionProjectNameTF.font();
            const QFontMetrics titleNameFm(titleFont);
            const int titleLineHeight = sessionProjectNameTF.lineHeight();
            const QFont pathFont = projectPathTF.font();
            const QFontMetrics pathFm(pathFont);
            const int pathLineHeight = projectPathTF.lineHeight();
            const int textWidth = bgR.right() - s(HPaddingXs) - textX;
            const auto getDisplayPath = [](const FilePath &p) {
                return p.osType() == OsTypeWindows ? p.displayName() : p.withTildeHomePath();
            };

            QString title;
            const FilePaths allPaths = pathsForSession(sessionName, &title);
            const qsizetype count = allPaths.size();
            const FilePaths paths = allPaths.first(std::min(kMaxPathsDisplay, count));
            const QStringList pathDisplay = Utils::transform(paths, getDisplayPath)
                                            + (count > kMaxPathsDisplay ? QStringList("...")
                                                                        : QStringList());
            if (pathDisplay.size()) {
                yy += s(VPaddingXxs);
                // title
                {
                    painter->setFont(titleFont);
                    painter->setPen(sessionProjectNameTF.color());
                    const QRect titleR(textX, yy, textWidth, titleLineHeight);
                    const QString titleElided
                        = titleNameFm.elidedText(title, Qt::ElideMiddle, textWidth);
                    painter->drawText(titleR, sessionProjectNameTF.drawTextFlags, titleElided);
                    yy += titleLineHeight;
                    yy += s(ExPaddingGapS);
                }
                {
                    painter->setFont(pathFont);
                    painter->setPen(projectPathTF.color());
                    for (const QString &displayPath : pathDisplay) {
                        const QRect pathR(textX, yy, textWidth, pathLineHeight);
                        const QString pathElided
                            = pathFm.elidedText(displayPath, Qt::ElideMiddle, textWidth);
                        painter->drawText(pathR, projectPathTF.drawTextFlags, pathElided);
                        yy += pathLineHeight;
                        yy += s(VPaddingXxs);
                    }
                }
            }
            yy += s(VGapXs);

            const QStringList actions = {
                Tr::tr("Clone"),
                Tr::tr("Rename"),
                Tr::tr("Delete"),
            };

            const QFont actionFont = actionTF.font();
            const QFontMetrics actionFm(actionTF.font());

            const int gapWidth = s(HGapXs) + actionSepWidth + s(HGapXs);
            int actionsTotalWidth = gapWidth * int(actions.count() - 1); // dividers
            const auto textWidths = Utils::transform(actions, [&] (const QString &action) {
                const int width = actionFm.horizontalAdvance(action);
                actionsTotalWidth += s(ExPaddingGapM) + width + s(ExPaddingGapM);
                return width;
            });

            const int buttonHeight = this->actionButtonHeight();
            int xx = (bgR.width() - actionsTotalWidth) / 2;
            for (int i = 0; i < actions.count(); ++i) {
                const QString &action = actions.at(i);
                const int ww = textWidths.at(i);
                const QRect actionR(xx, yy, s(ExPaddingGapM) + ww + s(ExPaddingGapM), buttonHeight);
                const bool isDisabled = i > 0 && SessionManager::isDefaultSession(sessionName);
                const bool isActive = actionR.adjusted(-s(VPaddingXs), 0, s(VPaddingXs) + 1, 0)
                                          .contains(mousePos) && !isDisabled;
                if (isActive) {
                    WelcomePageHelpers::drawCardBackground(painter, actionR, Qt::transparent,
                                                           creatorColor(Theme::Token_Text_Muted));
                    m_activeActionRects[i] = actionR;
                }
                painter->setFont(actionFont);
                painter->setPen((isDisabled ? actionDisabledTF : actionTF).color());
                const QRect actionTextR = actionR.adjusted(0, 0, 0, -1);
                painter->drawText(actionTextR, actionTF.drawTextFlags, action);
                xx += actionR.width();
                if (i < actions.count() - 1) {
                    const QRect dividerR(xx + s(HGapXs), yy, actionSepWidth, buttonHeight);
                    painter->fillRect(dividerR, creatorColor(Theme::Token_Text_Muted));
                }
                xx += gapWidth;
            }
            yy += buttonHeight;
            yy += s(VGapXs);
        }
        QTC_CHECK(option.rect.bottom() == yy + itemSpacing());
    }

    static int headerHeight()
    {
        const int paddingsHeight = s(VPaddingXs + VPaddingXs);
        const int heightForSessionName = sessionNameTF.lineHeight() + paddingsHeight;
        const int heightForIcon =
            withIcon() ? int(icon().deviceIndependentSize().height()) + paddingsHeight : 0;
        return qMax(heightForSessionName, heightForIcon);
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &idx) const final
    {
        int h = headerHeight();
        if (expanded(idx)) {
            const QString sessionName = idx.data(Qt::DisplayRole).toString();
            const FilePaths paths = pathsForSession(sessionName);
            const int displayPathCount = std::min(kMaxPathsDisplay + 1, paths.size());
            const int contentHeight
                = displayPathCount == 0
                      ? 0
                      : s(VPaddingXxs) + sessionProjectNameTF.lineHeight() + s(ExPaddingGapS)
                            + displayPathCount * (projectPathTF.lineHeight() + s(VPaddingXxs));
            h += contentHeight + s(VGapXs) + actionButtonHeight() + s(VGapXs);
        }
        return QSize(-1, h + itemSpacing());
    }

    bool editorEvent(QEvent *ev, QAbstractItemModel *model,
                     const QStyleOptionViewItem &, const QModelIndex &idx) final
    {
        if (ev->type() == QEvent::MouseButtonRelease) {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(ev);
            const Qt::MouseButtons button = mouseEvent->button();
            const QPoint pos = static_cast<QMouseEvent *>(ev)->pos();
            const QString sessionName = idx.data(Qt::DisplayRole).toString();
            if (m_activeExpandRect.contains(pos) || button == Qt::RightButton) {
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
            return false;
        }
        return false;
    }

private:
    QStringList m_expandedSessions;

    mutable QRect m_activeExpandRect;
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
        //                              visible on with Icon()               Extra margin right of project item
        //                                         |                                         |
        //                                +--------+-------+                          +------+-----+
        //                                |                |                          |            |
        //
        // +------------+--------+--------+------+---------+-------------+------------+------------+
        // |            |        |        |      |         | (VPaddingXs)|            |            |
        // |            |        |        |      |         +-------------+            |            |
        // |            |        |        |      |         |<projectName>|            |            |
        // |            |        |        |      |         +-------------+            |            |
        // |(HPaddingXs)|<number>|(HGapXs)|<icon>|(HGapXxs)|   (VGapXs)  |(HPaddingXs)|(HPaddingXs)|
        // |            |(w:6)   |        |      |         +-------------+            |            |
        // |            |        |        |      |         |<projectPath>|            |            |
        // |            |        |        |      |         +-------------+            |            |
        // |            |        |        |      |         | (VPaddingXs)|            |            |
        // +------------+--------+--------+------+---------+-------------+------------+------------+  --+
        // |                                        (VGapL)                                        |    +-- Gap between project items
        // +---------------------------------------------------------------------------------------+  --+

        const bool hovered = option.widget->isActiveWindow()
                             && option.state & QStyle::State_MouseOver;

        const QRect bgR = option.rect.adjusted(0, 0, -s(HPaddingXs), -itemSpacing());

        static const QPixmap icon = pixmap("project", Theme::Token_Text_Muted);
        const QSize iconS = icon.deviceIndependentSize().toSize();

        const int x = bgR.x();
        const int numberX = x + s(HPaddingXs);
        const int iconX = numberX + shortcutNumberWidth + s(HGapXs);
        const int iconWidth = iconS.width();
        const int textX = withIcon() ? iconX + iconWidth + s(HGapXs) : iconX;
        const int textWidth = bgR.width() - s(HPaddingXs) - textX;

        const int y = bgR.y();
        const int iconHeight = iconS.height();
        const int iconY = y + (bgR.height() - iconHeight) / 2;
        const int projectNameY = y + s(VPaddingXs);
        const QRect projectNameR(textX, projectNameY, textWidth, projectNameTF.lineHeight());
        const int projectPathY = projectNameY + projectNameR.height() + s(VGapXs);
        const QRect projectPathR(textX, projectPathY, textWidth, projectPathTF.lineHeight());

        QTC_CHECK(option.rect.bottom() == projectPathR.bottom() + s(VPaddingXs) + itemSpacing());

        {
            drawBackgroundRect(painter, bgR, hovered);
        }
        if (idx.row() < 9) {
            painter->setPen(shortcutNumberTF.color());
            painter->setFont(shortcutNumberTF.font());
            const QRect numberR(numberX, y, shortcutNumberWidth, bgR.height());
            const QString numberString = QString::number(idx.row() + 1);
            painter->drawText(numberR, shortcutNumberTF.drawTextFlags, numberString);
        }
        if (withIcon()) {
            painter->drawPixmap(iconX, iconY, icon);
        }
        {
            painter->setPen(projectNameTF.color());
            painter->setFont(projectNameTF.font(hovered));
            const QString projectName = idx.data(Qt::DisplayRole).toString();
            const QString projectNameElided =
                    painter->fontMetrics().elidedText(projectName, Qt::ElideRight, textWidth);
            painter->drawText(projectNameR, projectNameTF.drawTextFlags, projectNameElided);
        }
        {
            painter->setPen(projectPathTF.color());
            painter->setFont(projectPathTF.font());
            const FilePath projectPath =
                FilePath::fromVariant(idx.data(ProjectModel::FilePathRole));
            const QString displayPath =
                projectPath.osType() == OsTypeWindows ? projectPath.displayName()
                                                      : projectPath.withTildeHomePath();
            const QString displayPathElided =
                painter->fontMetrics().elidedText(displayPath, Qt::ElideMiddle, textWidth);
            painter->drawText(projectPathR, projectPathTF.drawTextFlags, displayPathElided);
        }
    }

    QSize sizeHint([[maybe_unused]] const QStyleOptionViewItem &option,
                   [[maybe_unused]] const QModelIndex &idx) const override
    {
        return QSize(-1, itemHeight() + itemSpacing());
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

private:
    static int itemHeight()
    {
        const int height =
            s(VPaddingXs)
            + projectNameTF.lineHeight()
            + s(VGapXs)
            + projectPathTF.lineHeight()
            + s(VPaddingXs);
        return height;
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
        setBackgroundColor(viewport(), Theme::Token_Background_Default);
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

        using namespace Layouting;

        auto sessions = new QWidget;
        {
            auto sessionsLabel = new Core::Label(Tr::tr("Sessions"), Core::Label::Primary);
            auto manageSessionsButton = new Button(Tr::tr("Manage..."), Button::MediumSecondary);
            auto sessionsList = new TreeView(this, "Sessions");
            sessionsList->setModel(projectWelcomePage->m_sessionModel);
            sessionsList->header()->setSectionHidden(1, true); // The "last modified" column.
            sessionsList->setItemDelegate(&m_sessionDelegate);
            sessionsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            QSizePolicy sessionsSp(QSizePolicy::Expanding, QSizePolicy::Expanding);
            sessionsSp.setHorizontalStretch(3);
            sessions->setSizePolicy(sessionsSp);
            Column {
                Row {
                    sessionsLabel,
                    st,
                    manageSessionsButton,
                    customMargins(HPaddingS, 0, sessionScrollBarGap, 0),
                },
                sessionsList,
                spacing(ExPaddingGapL),
                customMargins(ExVPaddingGapXl, ExVPaddingGapXl, 0, 0),
            }.attachTo(sessions);
            connect(manageSessionsButton, &Button::clicked,
                    this, &SessionManager::showSessionManager);
        }

        auto projects = new QWidget;
        {
            auto projectsLabel = new Core::Label(Tr::tr("Projects"), Core::Label::Primary);
            auto projectsList = new TreeView(this, "Recent Projects");
            projectsList->setUniformRowHeights(true);
            projectsList->setModel(projectWelcomePage->m_projectModel);
            projectsList->setItemDelegate(&m_projectDelegate);
            projectsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            QSizePolicy projectsSP(QSizePolicy::Expanding, QSizePolicy::Expanding);
            projectsSP.setHorizontalStretch(5);
            projects->setSizePolicy(projectsSP);
            Column {
                Row {
                    projectsLabel,
                    customMargins(HPaddingS, 0, 0, 0),
                },
                projectsList,
                spacing(ExPaddingGapL),
                customMargins(ExVPaddingGapXl - sessionScrollBarGap, ExVPaddingGapXl, 0, 0),
            }.attachTo(projects);
        }

        Row {
            sessions,
            projects,
            spacing(0),
            noMargin,
        }.attachTo(this);
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
