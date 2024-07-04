// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "introductionwidget.h"
#include "welcometr.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/welcomepagehelper.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/treemodel.h>

#include <QButtonGroup>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

using namespace Core;
using namespace Core::WelcomePageHelpers;
using namespace ExtensionSystem;
using namespace Utils;
using namespace StyleHelper::SpacingTokens;

namespace Welcome {
namespace Internal {

class TopArea;
class SideArea;

const char currentPageSettingsKeyC[] = "Welcome2Tab";

class WelcomeMode : public IMode
{
    Q_OBJECT

public:
    WelcomeMode();
    ~WelcomeMode();

    void initPlugins();

private:
    void addPage(IWelcomePage *page);

    ResizeSignallingWidget *m_modeWidget;
    QStackedWidget *m_pageStack;
    TopArea *m_topArea;
    SideArea *m_sideArea;
    QList<IWelcomePage *> m_pluginList;
    QList<QAbstractButton *> m_pageButtons;
    QButtonGroup *m_buttonGroup;
    Id m_activePage;
    Id m_defaultPage;
};

class WelcomePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Welcome.json")

public:
    ~WelcomePlugin() final { delete m_welcomeMode; }

    bool initialize(const QStringList &arguments, QString *) final
    {
        m_welcomeMode = new WelcomeMode;

        auto introAction = new QAction(Tr::tr("UI Tour"), this);
        connect(introAction, &QAction::triggered, this, []() {
            auto intro = new IntroductionWidget(ICore::dialogParent());
            intro->show();
        });
        Command *cmd = ActionManager::registerAction(introAction, "Welcome.UITour");
        ActionContainer *mhelp = ActionManager::actionContainer(Core::Constants::M_HELP);
        if (QTC_GUARD(mhelp))
            mhelp->addAction(cmd, Core::Constants::G_HELP_HELP);

        if (!arguments.contains("-notour")) {
            connect(ICore::instance(), &ICore::coreOpened, this, []() {
                IntroductionWidget::askUserAboutIntroduction(ICore::dialogParent());
            }, Qt::QueuedConnection);
        }

        return true;
    }

    void extensionsInitialized() final
    {
        m_welcomeMode->initPlugins();
        ModeManager::activateMode(m_welcomeMode->id());
    }

    WelcomeMode *m_welcomeMode = nullptr;
};

class TopArea : public QWidget
{
    Q_OBJECT

public:
    TopArea(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

        constexpr TextFormat welcomeTF {Theme::Token_Text_Default, StyleHelper::UiElementH2};

        auto ideIconLabel = new QLabel;
        {
            const QPixmap logo = Core::Icons::QTCREATORLOGO_BIG.pixmap();
            const int size = logo.width();
            const QRect cropR = size == 128 ? QRect(9, 22, 110, 84) : QRect(17, 45, 222, 166);
            const QPixmap croppedLogo = logo.copy(cropR);
            const int lineHeight = welcomeTF.lineHeight();
            const QPixmap scaledCroppedLogo =
                croppedLogo.scaledToHeight((lineHeight - 12) * croppedLogo.devicePixelRatioF(),
                                           Qt::SmoothTransformation);
            ideIconLabel->setPixmap(scaledCroppedLogo);
            ideIconLabel->setFixedHeight(lineHeight);
        }

        auto welcomeLabel = new QLabel(Tr::tr("Welcome to %1")
                                           .arg(QGuiApplication::applicationDisplayName()));
        {
            welcomeLabel->setFont(welcomeTF.font());
            QPalette pal = palette();
            pal.setColor(QPalette::WindowText, welcomeTF.color());
            welcomeLabel->setPalette(pal);
        }

        using namespace Layouting;

        Column {
            Row {
                ideIconLabel,
                welcomeLabel,
                st,
                spacing(ExVPaddingGapXl),
                customMargins(HPaddingM, VPaddingM, HPaddingM, VPaddingM),
            },
            createRule(Qt::Horizontal),
            noMargin, spacing(0),
        }.attachTo(this);
    }
};

class SideArea : public QScrollArea
{
    Q_OBJECT

public:
    SideArea(QWidget *parent = nullptr)
        : QScrollArea(parent)
    {
        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

        using namespace Layouting;

        Column mainColumn {
            spacing(0),
            customMargins(ExVPaddingGapXl, 0, ExVPaddingGapXl, 0),
        };

        m_essentials = new QWidget;
        Column essentials {
            spacing(0),
            noMargin,
        };

        {
            auto newButton = new Button(Tr::tr("Create Project..."), Button::MediumPrimary);
            auto openButton = new Button(Tr::tr("Open Project..."), Button::MediumSecondary);

            Column projectButtons {
                newButton,
                openButton,
                spacing(ExPaddingGapL),
                customMargins(0, ExVPaddingGapXl, 0, ExVPaddingGapXl),
            };

            essentials.addItem(projectButtons);

            connect(openButton, &Button::clicked, this, [] {
                QAction *openAction = ActionManager::command(Core::Constants::OPEN)->action();
                openAction->trigger();
            });
            connect(newButton, &Button::clicked, this, [] {
                QAction *openAction = ActionManager::command(Core::Constants::NEW)->action();
                openAction->trigger();
            });
        }

        {
            auto l = m_pluginButtons = new QVBoxLayout;
            l->setSpacing(VGapL);
            l->setContentsMargins({});
            essentials.addItem(l);
        }

        essentials.attachTo(m_essentials);
        mainColumn.addItem(m_essentials);
        mainColumn.addItem(st);

        {
            auto label = new Core::Label(Tr::tr("Explore more"), Core::Label::Secondary);
            label->setContentsMargins(HPaddingXxs, 0, 0, 0); // Is indented in Figma design

            Column linksLayout {
                label,
                spacing(VGapS),
                customMargins(0, VGapL, 0, ExVPaddingGapXl),
            };

            const struct {
                const QString label;
                const QString url;
            } links [] =
                {
                 { Tr::tr("Get Started"), "qthelp://org.qt-project.qtcreator/doc/creator-getting-started.html" },
                 { Tr::tr("Get Qt"), "https://www.qt.io/download" },
                 { Tr::tr("Qt Account"), "https://account.qt.io" },
                 { Tr::tr("Online Community"), "https://forum.qt.io" },
                 { Tr::tr("Blogs"), "https://planet.qt.io" },
                 { Tr::tr("User Guide"), "qthelp://org.qt-project.qtcreator/doc/index.html" },
                 };
            for (auto &link : links) {
                auto button = new Button(link.label, Button::SmallLink, this);
                connect(button, &Button::clicked, this, [link]{
                    QDesktopServices::openUrl(link.url);});
                button->setToolTip(link.url);
                static const QPixmap icon = Icon({{":/welcome/images/link.png",
                                                   Theme::Token_Accent_Default}},
                                                 Icon::Tint).pixmap();
                button->setPixmap(icon);
                linksLayout.addItem(button);
            }

            m_links = new QWidget;
            linksLayout.attachTo(m_links);
            mainColumn.addItem(m_links);
        }

        QWidget *mainWidget = new QWidget;

        Row {
            mainColumn,
            createRule(Qt::Vertical),
            noMargin, spacing(0),
        }.attachTo(mainWidget);

        setWidget(mainWidget);
    }

    QVBoxLayout *m_pluginButtons = nullptr;
    QWidget *m_essentials = nullptr;
    QWidget *m_links = nullptr;
};

WelcomeMode::WelcomeMode()
{
    setDisplayName(Tr::tr("Welcome"));

    const Icon CLASSIC(":/welcome/images/mode_welcome.png");
    const Icon FLAT({{":/welcome/images/mode_welcome_mask.png",
                      Theme::IconsBaseColor}});
    const Icon FLAT_ACTIVE({{":/welcome/images/mode_welcome_mask.png",
                             Theme::IconsModeWelcomeActiveColor}});
    setIcon(Icon::modeIcon(CLASSIC, FLAT, FLAT_ACTIVE));

    setPriority(Constants::P_MODE_WELCOME);
    setId(Constants::MODE_WELCOME);
    setContext(Context(Constants::C_WELCOME_MODE));

    m_modeWidget = new ResizeSignallingWidget;
    setBackgroundColor(m_modeWidget, Theme::Token_Background_Default);
    connect(m_modeWidget,
            &ResizeSignallingWidget::resized,
            this,
            [this](const QSize &size, const QSize &) {
                const QSize sideAreaS = m_sideArea->size();
                const QSize topAreaS = m_topArea->size();
                const QSize mainWindowS = ICore::mainWindow()->size();

                const bool showSideArea = sideAreaS.width() < size.width() / 4;
                const bool showTopArea = topAreaS.height() < mainWindowS.height() / 8.85;
                const bool showLinks = true;

                m_sideArea->m_links->setVisible(showLinks);
                m_sideArea->setVisible(showSideArea);
                m_topArea->setVisible(showTopArea);
            });

    m_sideArea = new SideArea(m_modeWidget);
    m_sideArea->verticalScrollBar()->setEnabled(false);

    m_buttonGroup = new QButtonGroup(m_modeWidget);
    m_buttonGroup->setExclusive(true);

    m_pageStack = new QStackedWidget(m_modeWidget);
    m_pageStack->setObjectName("WelcomeScreenStackedWidget");
    m_pageStack->setAutoFillBackground(true);

    m_topArea = new TopArea;

    using namespace Layouting;

    Column {
        new StyledBar,
        m_topArea,
        Row {
            m_sideArea,
            m_pageStack,
        },
        noMargin,
        spacing(0),
    }.attachTo(m_modeWidget);

    IContext::attach(m_modeWidget, {}, "Qt Creator Manual");
    setWidget(m_modeWidget);
}

WelcomeMode::~WelcomeMode()
{
    QtcSettings *settings = ICore::settings();
    settings->setValueWithDefault(currentPageSettingsKeyC,
                                  m_activePage.toSetting(),
                                  m_defaultPage.toSetting());
    delete m_modeWidget;
}

void WelcomeMode::initPlugins()
{
    QtcSettings *settings = ICore::settings();
    m_activePage = Id::fromSetting(settings->value(currentPageSettingsKeyC));

    for (IWelcomePage *page : IWelcomePage::allWelcomePages())
        addPage(page);

    if (!m_pageButtons.isEmpty()) {
        const int welcomeIndex = Utils::indexOf(m_pluginList,
                                                Utils::equal(&IWelcomePage::id,
                                                             Utils::Id("Examples")));
        const int defaultIndex = welcomeIndex >= 0 ? welcomeIndex : 0;
        m_defaultPage = m_pluginList.at(defaultIndex)->id();
        if (!m_activePage.isValid())
            m_pageButtons.at(defaultIndex)->click();
    }
}

void WelcomeMode::addPage(IWelcomePage *page)
{
    int idx;
    int pagePriority = page->priority();
    for (idx = 0; idx != m_pluginList.size(); ++idx) {
        if (m_pluginList.at(idx)->priority() >= pagePriority)
            break;
    }
    auto pageButton = new Button(page->title(), Button::SmallList, m_sideArea->widget());
    auto pageId = page->id();
    pageButton->setText(page->title());

    m_buttonGroup->addButton(pageButton);
    m_pluginList.insert(idx, page);
    m_pageButtons.insert(idx, pageButton);

    m_sideArea->m_pluginButtons->insertWidget(idx, pageButton);

    QWidget *stackPage = page->createWidget();
    stackPage->setAutoFillBackground(true);
    m_pageStack->insertWidget(idx, stackPage);

    connect(page, &QObject::destroyed, this, [this, page, stackPage, pageButton] {
        m_buttonGroup->removeButton(pageButton);
        m_pluginList.removeOne(page);
        m_pageButtons.removeOne(pageButton);
        delete pageButton;
        delete stackPage;
    });

    auto onClicked = [this, pageId, stackPage] {
        m_activePage = pageId;
        m_pageStack->setCurrentWidget(stackPage);
    };

    connect(pageButton, &Button::clicked, this, onClicked);
    if (pageId == m_activePage) {
        onClicked();
        pageButton->setChecked(true);
    }
}

} // namespace Internal
} // namespace Welcome

#include "welcomeplugin.moc"
