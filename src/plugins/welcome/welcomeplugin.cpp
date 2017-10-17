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

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <app/app_version.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/modemanager.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/treemodel.h>
#include <utils/theme/theme.h>

#include <QDesktopServices>
#include <QHeaderView>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPainter>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

using namespace Core;
using namespace ExtensionSystem;
using namespace Utils;

namespace Welcome {
namespace Internal {

class SideBar;

const int lrPadding = 34;
const char currentPageSettingsKeyC[] = "Welcome2Tab";

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

static QPalette lightText()
{
    QPalette pal;
    pal.setColor(QPalette::Foreground, themeColor(Theme::Welcome_ForegroundPrimaryColor));
    pal.setColor(QPalette::WindowText, themeColor(Theme::Welcome_ForegroundPrimaryColor));
    return pal;
}

class WelcomeMode : public IMode
{
    Q_OBJECT
public:
    WelcomeMode();
    ~WelcomeMode();

    void initPlugins();

    Q_INVOKABLE bool openDroppedFiles(const QList<QUrl> &urls);

private:
    void addPage(IWelcomePage *page);

    QWidget *m_modeWidget;
    QStackedWidget *m_pageStack;
    SideBar *m_sideBar;
    QList<IWelcomePage *> m_pluginList;
    QList<WelcomePageButton *> m_pageButtons;
    Id m_activePage;
};

class WelcomePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Welcome.json")

public:
    WelcomePlugin() {}

    bool initialize(const QStringList &, QString *) final
    {
        m_welcomeMode = new WelcomeMode;
        addAutoReleasedObject(m_welcomeMode);
        return true;
    }

    void extensionsInitialized() final
    {
        m_welcomeMode->initPlugins();
        ModeManager::activateMode(m_welcomeMode->id());
    }

    WelcomeMode *m_welcomeMode = nullptr;
};

class IconAndLink : public QWidget
{
public:
    IconAndLink(const QString &iconSource,
                const QString &title,
                const QString &openUrl,
                QWidget *parent)
        : QWidget(parent), m_iconSource(iconSource), m_title(title), m_openUrl(openUrl)
    {
        setAutoFillBackground(true);
        setMinimumHeight(30);
        setToolTip(m_openUrl);

        const QString fileName = QString(":/welcome/images/%1.png").arg(iconSource);
        const Icon icon({{fileName, Theme::Welcome_ForegroundPrimaryColor}}, Icon::Tint);

        m_icon = new QLabel;
        m_icon->setPixmap(icon.pixmap());

        m_label = new QLabel(title);
        m_label->setFont(sizedFont(11, m_label, false));

        auto layout = new QHBoxLayout;
        layout->setContentsMargins(lrPadding, 0, lrPadding, 0);
        layout->addWidget(m_icon);
        layout->addSpacing(2);
        layout->addWidget(m_label);
        layout->addStretch(1);
        setLayout(layout);
    }

    void enterEvent(QEvent *) override
    {
        QPalette pal;
        pal.setColor(QPalette::Background, themeColor(Theme::Welcome_HoverColor));
        setPalette(pal);
        m_label->setFont(sizedFont(11, m_label, true));
        update();
    }

    void leaveEvent(QEvent *) override
    {
        QPalette pal;
        pal.setColor(QPalette::Background, themeColor(Theme::Welcome_BackgroundColor));
        setPalette(pal);
        m_label->setFont(sizedFont(11, m_label, false));
        update();
    }

    void mousePressEvent(QMouseEvent *) override
    {
        QDesktopServices::openUrl(m_openUrl);
    }

    QString m_iconSource;
    QString m_title;
    const QString m_openUrl;

    QLabel *m_icon;
    QLabel *m_label;
};

class SideBar : public QWidget
{
    Q_OBJECT
public:
    SideBar(QWidget *parent)
        : QWidget(parent)
    {
        setAutoFillBackground(true);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        setPalette(themeColor(Theme::Welcome_BackgroundColor));

        auto vbox = new QVBoxLayout(this);
        vbox->setSpacing(0);
        vbox->setContentsMargins(0, 27, 0, 0);

        {
            auto l = m_pluginButtons = new QVBoxLayout;
            l->setContentsMargins(lrPadding, 0, lrPadding, 0);
            l->setSpacing(19);
            vbox->addItem(l);
            vbox->addSpacing(62);
        }

        {
            auto l = new QVBoxLayout;
            l->setContentsMargins(lrPadding, 0, lrPadding, 0);
            l->setSpacing(8);

            auto newLabel = new QLabel(tr("New to Qt?"), this);
            newLabel->setFont(sizedFont(18, this));
            l->addWidget(newLabel);

            auto learnLabel = new QLabel(tr("Learn how to develop your own applications "
                                            "and explore %1.")
                                         .arg(Core::Constants::IDE_DISPLAY_NAME), this);
            learnLabel->setMaximumWidth(200);
            learnLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            learnLabel->setWordWrap(true);
            learnLabel->setFont(sizedFont(12, this));
            learnLabel->setPalette(lightText());
            l->addWidget(learnLabel);

            l->addSpacing(12);

            auto getStartedButton = new WelcomePageButton(this);
            getStartedButton->setText(tr("Get Started Now"));
            getStartedButton->setOnClicked([] {
                QDesktopServices::openUrl(QString("qthelp://org.qt-project.qtcreator/doc/index.html"));
            });
            l->addWidget(getStartedButton);

            vbox->addItem(l);
            vbox->addSpacing(77);
        }

        {
            auto l = new QVBoxLayout;
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(5);
            l->addWidget(new IconAndLink("qtaccount", tr("Qt Account"), "https://account.qt.io", this));
            l->addWidget(new IconAndLink("community", tr("Online Community"), "http://forum.qt.io", this));
            l->addWidget(new IconAndLink("blogs", tr("Blogs"), "http://planet.qt.io", this));
            l->addWidget(new IconAndLink("userguide", tr("User Guide"),
                                         "qthelp://org.qt-project.qtcreator/doc/index.html", this));
            vbox->addItem(l);
        }

        vbox->addStretch(1);
    }

    QVBoxLayout *m_pluginButtons = nullptr;
};

WelcomeMode::WelcomeMode()
{
    setDisplayName(tr("Welcome"));

    const Icon CLASSIC(":/welcome/images/mode_welcome.png");
    const Icon FLAT({{":/welcome/images/mode_welcome_mask.png",
                      Theme::IconsBaseColor}});
    const Icon FLAT_ACTIVE({{":/welcome/images/mode_welcome_mask.png",
                             Theme::IconsModeWelcomeActiveColor}});
    setIcon(Icon::modeIcon(CLASSIC, FLAT, FLAT_ACTIVE));

    setPriority(Constants::P_MODE_WELCOME);
    setId(Constants::MODE_WELCOME);
    setContextHelpId("Qt Creator Manual");
    setContext(Context(Constants::C_WELCOME_MODE));

    QPalette palette = creatorTheme()->palette();
    palette.setColor(QPalette::Background, themeColor(Theme::Welcome_BackgroundColor));

    m_modeWidget = new QWidget;
    m_modeWidget->setPalette(palette);

    m_sideBar = new SideBar(m_modeWidget);
    auto scrollableSideBar = new QScrollArea(m_modeWidget);
    scrollableSideBar->setWidget(m_sideBar);
    scrollableSideBar->setWidgetResizable(true);
    scrollableSideBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollableSideBar->setFrameShape(QFrame::NoFrame);

    auto divider = new QWidget(m_modeWidget);
    divider->setMaximumWidth(1);
    divider->setMinimumWidth(1);
    divider->setAutoFillBackground(true);
    divider->setPalette(themeColor(Theme::Welcome_DividerColor));

    m_pageStack = new QStackedWidget(m_modeWidget);
    m_pageStack->setObjectName("WelcomeScreenStackedWidget");
    m_pageStack->setAutoFillBackground(true);

    auto hbox = new QHBoxLayout;
    hbox->addWidget(scrollableSideBar);
    hbox->addWidget(divider);
    hbox->addWidget(m_pageStack);
    hbox->setStretchFactor(m_pageStack, 10);

    auto layout = new QVBoxLayout(m_modeWidget);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(new StyledBar(m_modeWidget));
    layout->addItem(hbox);

    if (Utils::HostOsInfo::isMacHost()) { // workaround QTBUG-61384
        auto openglWidget = new QOpenGLWidget;
        openglWidget->hide();
        layout->addWidget(openglWidget);
    }

    setWidget(m_modeWidget);
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = ICore::settings();
    settings->setValue(currentPageSettingsKeyC, m_activePage.toSetting());
    delete m_modeWidget;
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = ICore::settings();
    m_activePage = Id::fromSetting(settings->value(currentPageSettingsKeyC));

    const QList<IWelcomePage *> availablePages = PluginManager::getObjects<IWelcomePage>();
    for (IWelcomePage *page : availablePages)
        addPage(page);

    // make sure later added pages are made available too:
    connect(PluginManager::instance(), &PluginManager::objectAdded, this, [this](QObject *obj) {
        if (IWelcomePage *page = qobject_cast<IWelcomePage*>(obj))
            addPage(page);
    });

    if (!m_activePage.isValid() && !m_pageButtons.isEmpty()) {
        m_activePage = m_pluginList.at(0)->id();
        m_pageButtons.at(0)->click();
    }
}

bool WelcomeMode::openDroppedFiles(const QList<QUrl> &urls)
{
//    DropArea {
//        anchors.fill: parent
//        keys: ["text/uri-list"]
//        onDropped: {
//            if ((drop.supportedActions & Qt.CopyAction != 0)
//                    && welcomeMode.openDroppedFiles(drop.urls))
//                drop.accept(Qt.CopyAction);
//        }
//    }
    const QList<QUrl> localUrls = Utils::filtered(urls, &QUrl::isLocalFile);
    if (!localUrls.isEmpty()) {
        QTimer::singleShot(0, [localUrls]() {
            ICore::openFiles(Utils::transform(localUrls, &QUrl::toLocalFile), ICore::SwitchMode);
        });
        return true;
    }
    return false;
}

void WelcomeMode::addPage(IWelcomePage *page)
{
    int idx;
    int pagePriority = page->priority();
    for (idx = 0; idx != m_pluginList.size(); ++idx) {
        if (m_pluginList.at(idx)->priority() >= pagePriority)
            break;
    }
    auto pageButton = new WelcomePageButton(m_sideBar);
    auto pageId = page->id();
    pageButton->setText(page->title());
    pageButton->setActiveChecker([this, pageId] { return m_activePage == pageId; });

    m_pluginList.append(page);
    m_pageButtons.append(pageButton);

    m_sideBar->m_pluginButtons->insertWidget(idx, pageButton);

    QWidget *stackPage = page->createWidget();
    stackPage->setAutoFillBackground(true);
    m_pageStack->insertWidget(idx, stackPage);

    auto onClicked = [this, pageId, stackPage] {
        m_activePage = pageId;
        m_pageStack->setCurrentWidget(stackPage);
        for (WelcomePageButton *pageButton : m_pageButtons)
            pageButton->recheckActive();
    };

    pageButton->setOnClicked(onClicked);
    if (pageId == m_activePage)
        onClicked();
}

} // namespace Internal
} // namespace Welcome

#include "welcomeplugin.moc"
