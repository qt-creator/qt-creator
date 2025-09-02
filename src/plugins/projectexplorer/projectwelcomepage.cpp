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
#include <utils/elidinglabel.h>
#include <utils/fileutils.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcwidgets.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QAbstractItemDelegate>
#include <QAction>
#include <QButtonGroup>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QToolButton>
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
constexpr TextFormat sessionTypeTF {Theme::Token_Text_Default, projectNameTF.uiElement};
constexpr TextFormat sessionProjectPathTF = projectPathTF;
constexpr TextFormat shortcutNumberTF {Theme::Token_Text_Default,
                                      StyleHelper::UiElementCaptionStrong,
                                      Qt::AlignCenter | Qt::TextDontClip};
constexpr int shortcutNumberWidth = 6;
constexpr int sessionScrollBarGap = HPaddingXs;
const int itemSpacing = VGapL;

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

    StyleHelper::drawCardBg(painter, rect, fill, pen, StyleHelper::defaultCardBgRounding);
}

class BaseDelegate : public QAbstractItemDelegate
{
public:
    static QString toolTip(const QModelIndex &idx, int shortcutRole, const QString &entryType)
    {
        const QString name = idx.data(Qt::DisplayRole).toString();
        const QString shortcut = idx.data(shortcutRole).toString();
        const QString text =
                shortcut.isEmpty() ? Tr::tr("Open %1 \"%2\"").arg(entryType, name)
                                   : Tr::tr("Open %1 \"%2\" (%3)").arg(entryType, name, shortcut);
        return text;
    }

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

        const QString toolTipText = toolTip(idx, shortcutRole(), entryType());
        if (toolTipText.isEmpty())
            return false;

        QToolTip::showText(ev->globalPos(), toolTipText, view);
        return true;
    }
};

class SessionItemExpansionButton final : public QAbstractButton
{
public:
    SessionItemExpansionButton()
    {
        setCheckable(true);
        setAutoFillBackground(false);
    }

    void paintEvent([[maybe_unused]] QPaintEvent *event) override
    {
        QPainter painter(this);
        const QRect bgR = rect().adjusted(-StyleHelper::defaultCardBgRounding, 0, 0,
                                          isChecked() ? StyleHelper::defaultCardBgRounding : 0);
        drawBackgroundRect(&painter, bgR, underMouse());

        static const QPixmap arrowDown =
            Icon({{FilePath::fromString(":/core/images/expandarrow.png"),
                   Theme::Token_Text_Muted}}, Icon::Tint).pixmap();
        static const QPixmap arrowUp =
            QPixmap::fromImage(arrowDown.toImage().mirrored(false, true));
        const QPixmap &arrow = isChecked() ? arrowUp : arrowDown;
        QRect arrowR(QPoint(), arrow.deviceIndependentSize().toSize());
        arrowR.moveCenter(rect().center());
        painter.drawPixmap(arrowR, arrow);
    }
};

class SessionItemWidget final : public QWidget
{
    Q_OBJECT
public:
    enum Actions {
        ActionClone,
        ActionRename,
        ActionDelete,
    };

    SessionItemWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_shortcut = new QLabel;
        applyTf(m_shortcut, shortcutNumberTF, false);
        const int shortcutWidth = HPaddingXs + shortcutNumberWidth + HGapXs;
        m_shortcut->setMinimumWidth(shortcutWidth);

        static const QPixmap icon = pixmap("session", Theme::Token_Text_Muted);
        const QSize iconS = icon.deviceIndependentSize().toSize();
        auto iconLabel = new QLabel;
        iconLabel->setFixedWidth(iconS.width());
        iconLabel->setPixmap(icon);

        m_sessionNameLabel = new ElidingLabel;
        applyTf(m_sessionNameLabel, sessionNameTF);
        m_sessionNameLabel->setElideMode(Qt::ElideMiddle);
        m_sessionNameLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_sessionNameLabel->setTextInteractionFlags(Qt::NoTextInteraction);

        const int collapsedHeight = VPaddingXs + std::max(16, sessionNameTF.lineHeight())
                                    + VPaddingXs;
        m_shortcut->setMinimumHeight(collapsedHeight);
        iconLabel->setMinimumHeight(collapsedHeight);

        m_expand = new SessionItemExpansionButton;
        m_expand->setFixedSize(32, collapsedHeight);

        m_sessionType = new ElidingLabel;
        applyTf(m_sessionType, sessionTypeTF);
        m_sessionType->setFixedHeight(m_sessionType->height() + VGapXs);
        m_sessionType->setSizePolicy(m_sessionNameLabel->sizePolicy());
        m_sessionType->setTextInteractionFlags(Qt::NoTextInteraction);

        m_clone = new QtcButton(Tr::tr("Clone"), QtcButton::SmallTertiary);
        m_rename = new QtcButton(Tr::tr("Rename"), QtcButton::SmallTertiary);
        m_delete = new QtcButton(Tr::tr("Delete"), QtcButton::SmallTertiary);

        auto buttonGroup = new QButtonGroup(this);
        buttonGroup->addButton(m_clone, ActionClone);
        buttonGroup->addButton(m_rename, ActionRename);
        buttonGroup->addButton(m_delete, ActionDelete);

        auto space = [] {
            auto sp = new QWidget;
            sp->setFixedSize(HGapXs, VGapXs);
            return sp;
        };

        using namespace Layouting;
        Column paths {
            m_sessionType,
            spacing(ExPaddingGapS),
            customMargins(0, 0, HPaddingXs, 0),
        };
        for (int i = 0; i < kMaxPathsDisplay + 1; ++i) {
            auto pathLine = new ElidingLabel;
            applyTf(pathLine, sessionProjectPathTF);
            pathLine->setElideMode(Qt::ElideMiddle);
            pathLine->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            pathLine->setTextInteractionFlags(Qt::NoTextInteraction);
            paths.addItem(pathLine);
            m_projectPaths.append(pathLine);
        }
        Column {
            Grid {
                m_shortcut, iconLabel, space(), m_sessionNameLabel, space(), m_expand, br,
                Span(3, empty), Span(3, paths), br,
            },
            Widget {
                bindTo(&m_buttons),
                Row {
                    st,
                    m_clone,
                    createRule(Qt::Vertical),
                    m_rename,
                    createRule(Qt::Vertical),
                    m_delete,
                    st,
                    spacing(ExPaddingGapM),
                    noMargin,
                },
                customMargins(0, VPaddingXs, 0, VPaddingXs),
            },
            customMargins(0, 0, sessionScrollBarGap, itemSpacing),
            spacing(0),
        }.attachTo(this);

        setAutoFillBackground(true);
        applyExpansion();

        connect(m_expand, &QAbstractButton::toggled, this, &SessionItemWidget::applyExpansion);
        connect(buttonGroup, &QButtonGroup::idClicked, this, &SessionItemWidget::actionRequested);
    }

    void setData(const QModelIndex &index)
    {
        const int row = index.row() + 1;
        m_shortcut->setText(row <= 9 ? QString::number(row) : QString());

        m_sessionName = index.data(Qt::DisplayRole).toString();

        const bool isLastSession = index.data(SessionModel::LastSessionRole).toBool();
        const bool isDefaultVirgin = SessionManager::isDefaultVirgin();
        const bool isActiveSession = index.data(SessionModel::ActiveSessionRole).toBool();
        QString fullSessionName = m_sessionName;
        if (isLastSession && isDefaultVirgin)
                fullSessionName = Tr::tr("%1 (last session)").arg(fullSessionName);
        if (isActiveSession && !isDefaultVirgin)
                fullSessionName = Tr::tr("%1 (current session)").arg(fullSessionName);
        m_sessionNameLabel->setText(fullSessionName);

        m_rename->setEnabled(!SessionManager::isDefaultSession(m_sessionName));
        m_delete->setEnabled(m_rename->isEnabled() && !isActiveSession);

        const QString entryType = Tr::tr("session", "Appears in \"Open session <name>\"");
        const QString toolTip = BaseDelegate::toolTip(index, SessionModel::ShortcutRole, entryType);
        setToolTip(toolTip);

        const auto getDisplayPath = [](const FilePath &p) {
            return p.osType() == OsTypeWindows ? p.displayName() : p.withTildeHomePath();
        };
        QString title;
        const FilePaths allPaths = pathsForSession(m_sessionName, &title);
        const qsizetype count = allPaths.size();
        const FilePaths paths = allPaths.first(std::min(kMaxPathsDisplay, count));
        const QStringList pathDisplay = Utils::transform(paths, getDisplayPath)
                                        + (count > kMaxPathsDisplay ? QStringList("...")
                                                                    : QStringList());
        m_sessionType->setText(title);
        for (int i = 0; QLabel *path : std::as_const(m_projectPaths)) {
            path->setText(i < count ? pathDisplay.at(i) : QString());
            i++;
        }

        m_expand->setChecked(expandedSessions().contains(m_sessionName));
    }

    QString sessionName() const
    {
        return m_sessionName;
    }

signals:
    void actionRequested(int action);
    void sizeChanged();

protected:
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::Enter || e->type() == QEvent::Leave) {
            update();
            return true;
        }
        return QWidget::event(e);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        switch (event->button()) {
        case Qt::LeftButton:
            SessionManager::loadSession(m_sessionName);
            return;
        case Qt::RightButton:
            m_expand->toggle();
            return;
        default:
            QWidget::mousePressEvent(event);
        }
    }

    void paintEvent([[maybe_unused]] QPaintEvent *event) override
    {
        const bool hovered = underMouse();

        QFont sessionNameFont = m_sessionNameLabel->font();
        sessionNameFont.setUnderline(hovered);
        m_sessionNameLabel->setFont(sessionNameFont);

        m_expand->setVisible(hovered || expanded());

        const QRect bgR = rect().adjusted(0, 0, -sessionScrollBarGap, -itemSpacing);
        QPainter painter(this);
        drawBackgroundRect(&painter, bgR, hovered);
    }

    bool expanded() const
    {
        return m_expand->isChecked();
    }

    void applyExpansion()
    {
        const bool isExpanded = expanded();
        if (isExpanded)
            expandedSessions().insert(m_sessionName);
        else
            expandedSessions().remove(m_sessionName);
        m_buttons->setVisible(isExpanded);
        m_sessionType->setVisible(isExpanded && anyOf(m_projectPaths, [](const QLabel *label) {
            return !label->text().isEmpty();
        }));
        for (auto &projectPath : m_projectPaths)
            projectPath->setVisible(isExpanded && !projectPath->text().isEmpty());
        emit sizeChanged();
    }

private:
    static QSet<QString> &expandedSessions()
    {
        static QSet<QString> sessions;
        return sessions;
    }

    QString m_sessionName;
    QLabel *m_shortcut;
    SessionItemExpansionButton *m_expand;
    QAbstractButton *m_clone;
    QAbstractButton *m_rename;
    QAbstractButton *m_delete;
    QWidget *m_buttons;
    ElidingLabel *m_sessionNameLabel;
    ElidingLabel *m_sessionType;
    QList<QLabel *> m_projectPaths;
};

class ProjectItemWidget final : public QWidget
{
public:
    ProjectItemWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_shortcut = new QLabel;
        applyTf(m_shortcut, shortcutNumberTF, false);
        const int shortcutWidth = HPaddingXs + shortcutNumberWidth + HGapXs;
        m_shortcut->setMinimumWidth(shortcutWidth);

        const QPixmap icon = pixmap("project", Theme::Token_Text_Muted);
        const QSize iconS = icon.deviceIndependentSize().toSize();
        auto iconLabel = new QLabel;
        iconLabel->setFixedSize(iconS);
        iconLabel->setPixmap(icon);

        m_projectName = new ElidingLabel;
        applyTf(m_projectName, projectNameTF);
        m_projectName->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        m_projectPath = new ElidingLabel;
        applyTf(m_projectPath, projectPathTF);
        m_projectPath->setElideMode(Qt::ElideMiddle);
        m_projectPath->setSizePolicy(m_projectName->sizePolicy());

        using namespace Layouting;
        Row {
            m_shortcut,
            iconLabel,
            Space(HGapXs),
            Column {
                m_projectName,
                m_projectPath,
                customMargins(0, VPaddingXs, 0, VPaddingXs),
                spacing(VGapXs),
            },
            customMargins(0, 0, HPaddingXs, 0),
            spacing(0),
        }.attachTo(this);

        setAutoFillBackground(false);
    }

    void setData(const QModelIndex &index)
    {
        const int row = index.row() + 1;
        m_shortcut->setText(row <= 9 ? QString::number(row) : QString());

        const QString projectName = index.data(Qt::DisplayRole).toString();
        m_projectName->setText(projectName);

        const FilePath projectPath =
            FilePath::fromVariant(index.data(ProjectModel::FilePathRole));
        const QString displayPath =
            projectPath.osType() == OsTypeWindows ? projectPath.displayName()
                                                  : projectPath.withTildeHomePath();
        m_projectPath->setText(displayPath);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option)
    {
        const bool hovered = option.widget->isActiveWindow()
                             && option.state & QStyle::State_MouseOver;

        const QRect bgRGlobal = option.rect.adjusted(0, 0, -HPaddingXs, -itemSpacing);
        const QRect bgR = bgRGlobal.translated(-option.rect.topLeft());

        QFont projectNameFont = m_projectName->font();
        projectNameFont.setUnderline(hovered);
        m_projectName->setFont(projectNameFont);
        setFixedWidth(bgR.width());

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(bgRGlobal.topLeft());

        drawBackgroundRect(painter, bgR, hovered);
        render(painter, bgR.topLeft(), {}, QWidget::DrawChildren);

        painter->restore();
    }

private:
    QLabel *m_shortcut;
    ElidingLabel *m_projectName;
    ElidingLabel *m_projectPath;
};

class ProjectDelegate : public BaseDelegate
{
public:
    explicit ProjectDelegate()
        : BaseDelegate()
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index)
        const override
    {
        m_itemWidget.setData(index);
        m_itemWidget.paint(painter, option);
    }

    QSize sizeHint([[maybe_unused]] const QStyleOptionViewItem &option,
                   [[maybe_unused]] const QModelIndex &index) const override
    {
        return {-1, m_itemWidget.minimumSizeHint().height() + itemSpacing};
    }

protected:
    QString entryType() override
    {
        return Tr::tr("project", "Appears in \"Open project <name>\"");
    }

    int shortcutRole() const override
    {
        return ProjectModel::ShortcutRole;
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
    mutable ProjectItemWidget m_itemWidget;
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
        : m_projectWelcomePage(projectWelcomePage)
    {
        // FIXME: Remove once facilitateQml() is gone.
        if (!m_projectWelcomePage->m_sessionModel)
            m_projectWelcomePage->m_sessionModel = new SessionModel(this);
        if (!m_projectWelcomePage->m_projectModel)
            m_projectWelcomePage->m_projectModel = new ProjectModel(this);

        using namespace Layouting;

        auto sessions = new QWidget;
        {
            auto sessionsLabel = new QtcLabel(Tr::tr("Sessions"), QtcLabel::Primary);
            auto manageSessionsButton = new QtcButton(Tr::tr("Manage..."),
                                                      QtcButton::LargeSecondary);
            m_sessionList = new TreeView(this, "Sessions");
            m_sessionList->setModel(m_projectWelcomePage->m_sessionModel);
            m_sessionList->header()->setSectionHidden(1, true); // The "last modified" column.
            m_sessionList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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
                m_sessionList,
                spacing(ExPaddingGapL),
                customMargins(ExVPaddingGapXl, ExVPaddingGapXl, 0, 0),
            }.attachTo(sessions);
            connect(manageSessionsButton, &QtcButton::clicked,
                    this, &SessionManager::showSessionManager);
            connect(m_projectWelcomePage->m_sessionModel, &QAbstractItemModel::modelReset,
                    this, &SessionsPage::syncModelView);
        }

        auto projects = new QWidget;
        {
            auto projectsLabel = new QtcLabel(Tr::tr("Projects"), QtcLabel::Primary);
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

private:
    void syncModelView()
    {
        const int rowsCount = m_projectWelcomePage->m_sessionModel->rowCount();
        for (int row = 0; row < rowsCount; ++row) {
            const QModelIndex index = m_projectWelcomePage->m_sessionModel->index(row, 0);
            auto widget = qobject_cast<SessionItemWidget*>(m_sessionList->indexWidget(index));
            if (!widget) {
                widget = new SessionItemWidget;
                m_sessionList->setIndexWidget(index, widget);
                connect(widget, &SessionItemWidget::sizeChanged, this, [this]() {
                    m_sessionList->setVisible(false); // Hack to avoid layouting "flicker"
                    emit m_projectWelcomePage->m_sessionModel->layoutChanged();
                    m_sessionList->setVisible(true);
                });
                connect(widget, &SessionItemWidget::actionRequested, this, [this, widget](int action) {
                    switch (action) {
                    case SessionItemWidget::ActionClone:
                        m_projectWelcomePage->m_sessionModel->cloneSession(widget->sessionName());
                        break;
                    case SessionItemWidget::ActionRename:
                        m_projectWelcomePage->m_sessionModel->renameSession(widget->sessionName());
                        break;
                    case SessionItemWidget::ActionDelete:
                        m_projectWelcomePage->m_sessionModel->deleteSessions({(widget->sessionName())});
                        break;
                    };
                });
            }
            widget->setData(index);
        }
    }

    ProjectDelegate m_projectDelegate;
    ProjectWelcomePage* const m_projectWelcomePage;
    TreeView *m_sessionList;
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

#include "projectwelcomepage.moc"
