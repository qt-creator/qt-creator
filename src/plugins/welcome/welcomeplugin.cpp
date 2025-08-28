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
#include <coreplugin/session.h>
#include <coreplugin/welcomepagehelper.h>

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcwidgets.h>
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

namespace Welcome::Internal {

const char currentPageSettingsKeyC[] = "Welcome2Tab";

class TopArea final : public QWidget
{
public:
    TopArea(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

        m_pluginButtons = new QHBoxLayout;
        m_pluginButtons->setSpacing(VGapL);
        m_pluginButtons->setContentsMargins({});

        auto newButton = new QtcButton(Tr::tr("Create Project..."), QtcButton::LargePrimary);
        auto openButton = new QtcButton(Tr::tr("Open Project..."), QtcButton::LargeSecondary);

        using namespace Layouting;
        Column {
            Row {
                m_pluginButtons,
                st,
                newButton,
                openButton,
                spacing(HGapM),
                customMargins(HPaddingM, VPaddingM, HPaddingM, VPaddingM),
            },
            createRule(Qt::Horizontal),
            noMargin, spacing(0),
        }.attachTo(this);

        connect(openButton, &QtcButton::clicked, this, [] {
            QAction *openAction = ActionManager::command(Core::Constants::OPEN)->action();
            openAction->trigger();
        });
        connect(newButton, &QtcButton::clicked, this, [] {
            QAction *openAction = ActionManager::command(Core::Constants::NEW)->action();
            openAction->trigger();
        });
    }

    QHBoxLayout *m_pluginButtons = nullptr;
};

class WelcomeModeWidget final : public QWidget
{
public:
    WelcomeModeWidget()
    {
        setBackgroundColor(this, Theme::Token_Background_Default);

        m_buttonGroup = new QButtonGroup(this);
        m_buttonGroup->setExclusive(true);

        m_pageStack = new QStackedWidget(this);
        m_pageStack->setObjectName("WelcomeScreenStackedWidget");
        m_pageStack->setAutoFillBackground(true);

        m_topArea = new TopArea;

        using namespace Layouting;
        Column {
            new StyledBar,
            m_topArea,
            m_pageStack,
            noMargin,
            spacing(0),
        }.attachTo(this);

        IContext::attach(this, {}, "Qt Creator Manual");
    }

    ~WelcomeModeWidget()
    {
        QtcSettings *settings = ICore::settings();
        settings->setValueWithDefault(currentPageSettingsKeyC,
                                      m_activePage.toSetting(),
                                      m_defaultPage.toSetting());
    }

    void initPlugins()
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

    void addPage(IWelcomePage *page)
    {
        int idx;
        int pagePriority = page->priority();
        for (idx = 0; idx != m_pluginList.size(); ++idx) {
            if (m_pluginList.at(idx)->priority() >= pagePriority)
                break;
        }
        auto pageButton = new QtcButton(page->title(), QtcButton::SmallList, m_topArea);
        auto pageId = page->id();
        pageButton->setText(page->title());

        m_buttonGroup->addButton(pageButton);
        m_pluginList.insert(idx, page);
        m_pageButtons.insert(idx, pageButton);

        m_topArea->m_pluginButtons->insertWidget(idx, pageButton);

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

        connect(pageButton, &QtcButton::clicked, this, onClicked);
        if (pageId == m_activePage) {
            onClicked();
            pageButton->setChecked(true);
        }
    }

    QStackedWidget *m_pageStack;
    TopArea *m_topArea;
    QList<IWelcomePage *> m_pluginList;
    QList<QAbstractButton *> m_pageButtons;
    QButtonGroup *m_buttonGroup;

    Id m_activePage;
    Id m_defaultPage;
};

class WelcomeMode final : public IMode
{
public:
    WelcomeMode()
    {
        setDisplayName(Tr::tr("Welcome"));

        const Icon CLASSIC(":/welcome/images/mode_welcome.png");
        const Icon FLAT({{":/welcome/images/mode_welcome_mask.png",
                          Theme::IconsBaseColor}});
        setIcon(Icon::sideBarIcon(CLASSIC, FLAT));

        setPriority(Constants::P_MODE_WELCOME);
        setId(Constants::MODE_WELCOME);
        setContext(Context(Constants::C_WELCOME_MODE));

        m_modeWidget = new WelcomeModeWidget;
        setWidget(m_modeWidget);
    }

    ~WelcomeMode() { delete m_modeWidget; }

    void extensionsInitialized()
    {
        m_modeWidget->initPlugins();
        if (!SessionManager::loadsSessionOrFileAtStartup())
            ModeManager::activateMode(id());
    }

private:
    WelcomeModeWidget *m_modeWidget;
};

class WelcomePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Welcome.json")

    ~WelcomePlugin() final { delete m_welcomeMode; }

    Result<> initialize(const QStringList &arguments) final
    {
        m_welcomeMode = new WelcomeMode;

        ActionBuilder(this, "Welcome.UITour")
            .setText(Tr::tr("UI Tour"))
            .addToContainer(Core::Constants::M_HELP, Core::Constants::G_HELP_HELP, true)
            .addOnTriggered(&runUiTour);

        if (!arguments.contains("-notour")) {
            connect(ICore::instance(), &ICore::coreOpened, this, [] { askUserAboutIntroduction(); },
            Qt::QueuedConnection);
        }
        return ResultOk;
    }

    void extensionsInitialized() final
    {
        m_welcomeMode->extensionsInitialized();
    }

    WelcomeMode *m_welcomeMode = nullptr;
};

} // Welcome::Internal

#include "welcomeplugin.moc"
