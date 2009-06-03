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
#include "icore.h"
#include "iwizard.h"
#include "coreconstants.h"
#include "uniqueidmanager.h"
#include "modemanager.h"
#include "newdialog.h"
#include "rssfetcher.h"

#include <QtGui/QToolBar>
#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QSettings>

#include <cstdlib>

#include "ui_welcomemode.h"

namespace Core {
namespace Internal {

struct WelcomeModePrivate
{
    WelcomeModePrivate();

    QWidget *m_widget;
    QWidget *m_welcomePage;
    QButtonGroup *btnGrp;
    Ui::WelcomePage ui;
    RSSFetcher *rssFetcher;
    WelcomeMode::WelcomePageData lastData;
    int currentTip;
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

// Format a title + ruler for title labels
static inline QString titleLabel(const QString &text)
{
    QString  rc = QLatin1String(
    "<html><head><style type=\"text/css\">p, li { white-space: pre-wrap; }</style></head>"
    "<body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">"
    "<p style=\" margin-top:16px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
    "<span style=\" font-size:x-large; color:#555555;\">");
    rc += text;
    rc += QLatin1String("</span></p><hr/></body></html>");
    return rc;
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
    m_d->rssFetcher = new RSSFetcher(7, this);
    m_d->m_welcomePage = new QWidget(m_d->m_widget);
    m_d->ui.setupUi(m_d->m_welcomePage);
    m_d->ui.projTitleLabel->setText(titleLabel(tr("Projects")));
    m_d->ui.recentSessionsTitleLabel->setText(titleLabel(tr("Sessions")));
    m_d->ui.tutorialsTitleLabel->setText(titleLabel(tr("Tutorials")));
    m_d->ui.demoTitleLabel->setText(titleLabel(tr("Explore Qt Examples")));
    m_d->ui.didYouKnowTitleLabel->setText(titleLabel(tr("Did you know?")));
    m_d->ui.labsTitleLabel->setText(titleLabel(tr("News from the Qt Labs")));
    m_d->ui.sitesTitleLabel->setText(titleLabel(tr("Qt Websites")));
    m_d->ui.sessTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.projTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.newsTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.sitesTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.tutorialTreeWidget->viewport()->setAutoFillBackground(false);
    m_d->ui.didYouKnowTextBrowser->viewport()->setAutoFillBackground(false);
    m_d->ui.helpUsLabel->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_d->ui.feedbackButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    l->addWidget(m_d->m_welcomePage);

    updateWelcomePage(WelcomePageData());

    m_d->btnGrp = new QButtonGroup(this);
    m_d->btnGrp->addButton(m_d->ui.gettingStartedSectButton, 0);
    m_d->btnGrp->addButton(m_d->ui.developSectButton, 1);
    m_d->btnGrp->addButton(m_d->ui.communitySectButton, 2);

    connect(m_d->btnGrp, SIGNAL(buttonClicked(int)), m_d->ui.stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_d->ui.feedbackButton, SIGNAL(clicked()), SLOT(slotFeedback()));
    connect(m_d->ui.manageSessionsButton, SIGNAL(clicked()), SIGNAL(manageSessions()));
    connect(m_d->ui.createNewProjectButton, SIGNAL(clicked()), SLOT(slotCreateNewProject()));
    connect(m_d->ui.sessTreeWidget, SIGNAL(activated(QString)), SLOT(slotSessionClicked(QString)));
    connect(m_d->ui.projTreeWidget, SIGNAL(activated(QString)), SLOT(slotProjectClicked(QString)));
    connect(m_d->ui.newsTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(m_d->ui.sitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(m_d->ui.tutorialTreeWidget, SIGNAL(activated(QString)), SIGNAL(openHelpPage(const QString&)));
    connect(m_d->ui.openExampleButton, SIGNAL(clicked()), SLOT(slotOpenExample()));
    connect(m_d->ui.examplesComboBox, SIGNAL(currentIndexChanged(int)), SLOT(slotEnableExampleButton(int)));

    connect(this, SIGNAL(updatedExamples(QString, QString, QString)),
            this, SLOT(slotUpdateExamples(QString, QString, QString)));

    connect(m_d->rssFetcher, SIGNAL(newsItemReady(QString, QString, QString)),
        m_d->ui.newsTreeWidget, SLOT(slotAddNewsItem(QString, QString, QString)));

    //: Add localized feed here only if one exists
    m_d->rssFetcher->fetch(QUrl(tr("http://labs.trolltech.com/blogs/feed")));

    m_d->ui.sitesTreeWidget->addItem(tr("Qt Software"), QLatin1String("http://www.trolltech.com"));
    m_d->ui.sitesTreeWidget->addItem(tr("Qt Labs"), QLatin1String("http://labs.trolltech.com"));
    m_d->ui.sitesTreeWidget->addItem(tr("Qt Git Hosting"), QLatin1String("http://qt.gitorious.org"));
    m_d->ui.sitesTreeWidget->addItem(tr("Qt Centre"), QLatin1String("http://www.qtcentre.org"));
    m_d->ui.sitesTreeWidget->addItem(tr("Qt for S60 at Forum Nokia"), QLatin1String("http://discussion.forum.nokia.com/forum/forumdisplay.php?f=196"));

    m_d->ui.tutorialTreeWidget->addItem(tr("<b>Qt Creator - A quick tour</b>"),
                                        QString("qthelp://com.nokia.qtcreator.%1%2/doc/index.html").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR));
    m_d->ui.tutorialTreeWidget->addItem(tr("Creating an address book"),
                                        QLatin1String("qthelp://com.trolltech.qt/qdoc/tutorials-addressbook.html"));
    m_d->ui.tutorialTreeWidget->addItem(tr("Understanding widgets"),
                                        QLatin1String("qthelp://com.trolltech.qt/qdoc/widgets-tutorial.html"));
    m_d->ui.tutorialTreeWidget->addItem(tr("Building with qmake"),
                                        QLatin1String("qthelp://com.trolltech.qmake/qdoc/qmake-tutorial.html"));
    m_d->ui.tutorialTreeWidget->addItem(tr("Writing test cases"),
                                        QLatin1String("qthelp://com.trolltech.qt/qdoc/qtestlib-tutorial.html"));

    srand(QDateTime::currentDateTime().toTime_t());
    QStringList tips = tipsOfTheDay();
    m_d->currentTip = rand()%tips.count();

    QTextDocument *doc = m_d->ui.didYouKnowTextBrowser->document();
    doc->setDefaultStyleSheet("a:link {color:black;}");
    m_d->ui.didYouKnowTextBrowser->setDocument(doc);
    m_d->ui.didYouKnowTextBrowser->setText(tips.at(m_d->currentTip));

    connect(m_d->ui.nextTipBtn, SIGNAL(clicked()), this, SLOT(slotNextTip()));
    connect(m_d->ui.prevTipBtn, SIGNAL(clicked()), this, SLOT(slotPrevTip()));

    QSettings *settings = ICore::instance()->settings();
    int id = settings->value("General/WelcomeTab", 0).toInt();
    m_d->btnGrp->button(id)->setChecked(true);
    m_d->ui.stackedWidget->setCurrentIndex(id);
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = ICore::instance()->settings();
    settings->setValue("General/WelcomeTab", m_d->btnGrp->checkedId());
    delete m_d->m_widget;
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

void WelcomeMode::slotUrlClicked(const QString &data)
{
    QDesktopServices::openUrl(QUrl(data));
}

void WelcomeMode::slotUpdateExamples(const QString& examplePath, const QString& demosPath, const QString& docPath)
{
    QString demoxml = demosPath + "/qtdemo/xml/examples.xml";
    if (!QFile::exists(demoxml))
        return;

    QFile description(demoxml);
    if (!description.open(QFile::ReadOnly))
        return;

    m_d->ui.examplesComboBox->clear();
    m_d->ui.examplesComboBox->setEnabled(true);

    m_d->ui.examplesComboBox->addItem(tr("Choose an example..."));
    QFont f = widget()->font();
    f.setItalic(true);
    m_d->ui.examplesComboBox->setItemData(0, f, Qt::FontRole);
    f.setItalic(false);
    bool inExamples = false;
    QString dirName;
    QXmlStreamReader reader(&description);
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
            if (reader.name() == "category") {
                QString name = reader.attributes().value(QLatin1String("name")).toString();
                if (name.contains("tutorial"))
                    break;
                dirName = reader.attributes().value(QLatin1String("dirname")).toString();
                m_d->ui.examplesComboBox->addItem(name);
                f.setBold(true);
                m_d->ui.examplesComboBox->setItemData(m_d->ui.examplesComboBox->count()-1, f, Qt::FontRole);
                f.setBold(false);
                inExamples = true;
            }
            if (inExamples && reader.name() == "example") {
                QString name = reader.attributes().value(QLatin1String("name")).toString();
                QString fn = reader.attributes().value(QLatin1String("filename")).toString();
                QString fileName = examplePath + '/' + dirName + '/' + fn + '/' + fn + ".pro";
                QString helpPath = "qthelp://com.trolltech.qt/qdoc/" + dirName.replace("/", "-") + "-" + fn + ".html";

                m_d->ui.examplesComboBox->addItem("  " + name, fileName);
                m_d->ui.examplesComboBox->setItemData(m_d->ui.examplesComboBox->count()-1, helpPath, Qt::UserRole+1);
            }
            break;
            case QXmlStreamReader::EndElement:
            if (reader.name() == "category")
                inExamples = false;
            break;
            default:
            break;
        }
    }
}

void WelcomeMode::slotEnableExampleButton(int index)
{
    QString fileName = m_d->ui.examplesComboBox->itemData(index, Qt::UserRole).toString();
    m_d->ui.openExampleButton->setEnabled(!fileName.isEmpty());
}

void WelcomeMode::slotOpenExample()
{
    QComboBox *box = m_d->ui.examplesComboBox;
    QString proFile = box->itemData(box->currentIndex(), Qt::UserRole).toString();
    QString helpFile = box->itemData(box->currentIndex(), Qt::UserRole + 1).toString();
    QStringList files;
    QFileInfo fi(proFile);
    QString tryFile = fi.path() + "/main.cpp";
    files << proFile;
    if(!QFile::exists(tryFile))
        tryFile = fi.path() + '/' + fi.baseName() + ".cpp";
    if(QFile::exists(tryFile))
        files << tryFile;
    Core::ICore::instance()->openFiles(files);
    emit openContextHelpPage(helpFile);
}

void WelcomeMode::slotFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
            "http://www.trolltech.com/forms/feedback-forms/qt-creator-user-feedback/view")));
}

void WelcomeMode::slotCreateNewProject()
{
    Core::ICore::instance()->showNewItemDialog(tr("New Project..."),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}

void WelcomeMode::slotNextTip()
{
    QStringList tips = tipsOfTheDay();
    m_d->currentTip = ((m_d->currentTip+1)%tips.count());
    m_d->ui.didYouKnowTextBrowser->setText(tips.at(m_d->currentTip));
}

void WelcomeMode::slotPrevTip()
{
    QStringList tips = tipsOfTheDay();
    m_d->currentTip = ((m_d->currentTip-1)+tips.count())%tips.count();
    m_d->ui.didYouKnowTextBrowser->setText(tips.at(m_d->currentTip));
}

QStringList WelcomeMode::tipsOfTheDay()
{
    static QStringList tips;
    if (tips.isEmpty()) {
        tips.append(tr("You can switch between Qt Creator's modes using <tt>Ctrl+number</tt>:"
                       "<ol><li> - Welcome</li><li> - Edit</li><li>- Debug</li><li>- Projects</li><li>- Help</li>"
                       "<li></li><li>- Output</li></ol>"));
        tips.append(tr("You can show and hide the side bar using <tt>Alt+0<tt>."));
        tips.append(tr("You can fine tune the <tt>Find</tt> function by selecting &quot;Whole Words&quot; "
                       "or &quot;Case Sensitive&quot;. Simply click on the icons on the right end of the line edit."));                       
        tips.append(tr("If you add <a href=\"qthelp://com.nokia.qtcreator/doc/creator-external-library-handling.html\""
                       ">external libraries</a>, Qt Creator will automatically offer syntax highlighting "
                        "and code completion."));
    }
    return tips;
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
    QLabel *lbl = new QLabel(label);
    lbl->setTextInteractionFlags(Qt::NoTextInteraction);
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

void WelcomeModeTreeWidget::slotAddNewsItem(const QString &title, const QString &description, const QString &link)
{
    int itemWidth = width()-header()->sectionSize(0);
    QFont f = font();
    QString elidedText = QFontMetrics(f).elidedText(description, Qt::ElideRight, itemWidth);
    f.setBold(true);
    QString elidedTitle = QFontMetrics(f).elidedText(title, Qt::ElideRight, itemWidth);
    QString data = QString::fromLatin1("<b>%1</b><br />%2").arg(elidedTitle).arg(elidedText);
    addTopLevelItem(addItem(data,link));
}

void WelcomeModeTreeWidget::slotItemClicked(QTreeWidgetItem *item)
{
    emit activated(item->data(0, Qt::UserRole).toString());
}

} // namespace Internal
} // namespace Core
