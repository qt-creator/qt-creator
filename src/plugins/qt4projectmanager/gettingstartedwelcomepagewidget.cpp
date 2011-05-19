/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gettingstartedwelcomepagewidget.h"
#include "ui_gettingstartedwelcomepagewidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/rssfetcher.h>
#include <coreplugin/dialogs/iwizard.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QStringBuilder>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QScopedPointer>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFont>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QDesktopServices>

namespace Qt4ProjectManager {
namespace Internal {

const char ExamplePathPropertyName[] = "__qt_ExamplePath";
const char HelpPathPropertyName[] = "__qt_HelpPath";
const char QmlMainFileName[] = "__qt_QmlMainFileName";

void PixmapDownloader::populatePixmap(QNetworkReply *reply) {
    QImage image;
    image.loadFromData(reply->readAll());
    m_label->setScaledContents(false);
    m_label->setPixmap(QPixmap::fromImage(image));
    deleteLater();
}

GettingStartedWelcomePageWidget::GettingStartedWelcomePageWidget(QWidget *parent) :
    QWidget(parent), ui(new Ui::GettingStartedWelcomePageWidget),
    m_currentFeature(0), m_rssFetcher(0)
{
    ui->setupUi(this);

    ui->didYouKnowTextBrowser->viewport()->setAutoFillBackground(false);
    ui->detailsLabel->hide();

    connect(ui->tutorialTreeWidget, SIGNAL(activated(QString)), SLOT(slotOpenHelpPage(const QString&)));

    QFontMetrics fm = fontMetrics();
    const int margins = 30;
    int width = ui->tutorialTreeWidget->minimumWidth() - margins;

    QString itemText = tr("The Qt Creator User Interface");
    QString url = QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-quick-tour.html");
    ui->tutorialTreeWidget->addItem(fm.elidedText(itemText, Qt::ElideRight, width), url, itemText);

    itemText = tr("Building and Running an Example");
    url = QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-build-example-application.html?view=split");
    ui->tutorialTreeWidget->addItem(fm.elidedText(itemText, Qt::ElideRight, width), url, itemText);

    itemText = tr("Creating a Qt C++ Application");
    url = QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-writing-program.html?view=split");
    ui->tutorialTreeWidget->addItem(fm.elidedText(itemText, Qt::ElideRight, width), url, itemText);

    itemText = tr("Creating a Mobile Application");
    url = QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-mobile-example.html?view=split");
    ui->tutorialTreeWidget->addItem(fm.elidedText(itemText, Qt::ElideRight, width), url, itemText);

    itemText = tr("Creating a Qt Quick Application");
    url = QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-qml-application.html?view=split");
    ui->tutorialTreeWidget->addItem(fm.elidedText(itemText, Qt::ElideRight, width), url, itemText);

    srand(QDateTime::currentDateTime().toTime_t());
    QStringList tips = tipsOfTheDay();
    m_currentTip = rand()%tips.count();

    QTextDocument *doc = ui->didYouKnowTextBrowser->document();
    doc->setDefaultStyleSheet("* {color:black;};");
    ui->didYouKnowTextBrowser->setDocument(doc);
    ui->didYouKnowTextBrowser->setText(tips.at(m_currentTip));

    connect(ui->nextTipBtn, SIGNAL(clicked()), this, SLOT(slotNextTip()));
    connect(ui->prevTipBtn, SIGNAL(clicked()), this, SLOT(slotPrevTip()));
    connect(ui->openProjectButton, SIGNAL(clicked()),
            ProjectExplorer::ProjectExplorerPlugin::instance(),
            SLOT(openOpenProjectDialog()));
    connect(ui->createNewProjectButton, SIGNAL(clicked()), this, SLOT(slotCreateNewProject()));

    ui->createNewProjectButton->setIcon(
            QIcon::fromTheme(QLatin1String("document-new"), ui->createNewProjectButton->icon()));
    ui->openProjectButton->setIcon(
            QIcon::fromTheme(QLatin1String("document-open"), ui->openProjectButton->icon()));

    m_rssFetcher = new Core::RssFetcher;
    connect (m_rssFetcher, SIGNAL(rssItemReady(Core::RssItem)), SLOT(addToFeatures(Core::RssItem)));
    connect (m_rssFetcher, SIGNAL(finished(bool)), SLOT(showFeature()), Qt::QueuedConnection);
    connect(this, SIGNAL(startRssFetching(QUrl)), m_rssFetcher, SLOT(fetch(QUrl)), Qt::QueuedConnection);
    m_rssFetcher->start(QThread::LowestPriority);
    const QString featureRssFile = Core::ICore::instance()->resourcePath()+QLatin1String("/rss/featured.rss");
    emit startRssFetching(QUrl::fromLocalFile(featureRssFile));

    ui->nextFeatureBtn->hide();
    ui->prevFeatureBtn->hide();
    connect(ui->nextFeatureBtn, SIGNAL(clicked()), this, SLOT(slotNextFeature()));
    connect(ui->prevFeatureBtn, SIGNAL(clicked()), this, SLOT(slotPrevFeature()));
}

GettingStartedWelcomePageWidget::~GettingStartedWelcomePageWidget()
{
    m_rssFetcher->exit();
    m_rssFetcher->wait();
    delete m_rssFetcher;
    delete ui;
}

void GettingStartedWelcomePageWidget::parseXmlFile(QFile *file, QMenuHash &cppSubMenuHash, QMenuHash &qmlSubMenuHash,
                                                   const QString &examplePath, const QString &sourcePath)
{
    QMenu *cppSubMenu = 0;
    QMenu *qmlSubMenu = 0;
    bool inExamples = false;
    QString dirName;

    QXmlStreamReader reader(file);

    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("category")) {
                QString name = reader.attributes().value(QLatin1String("name")).toString();
                if (name.contains(QLatin1String("Tutorial")))
                    break;
                dirName = reader.attributes().value(QLatin1String("dirname")).toString();
                if (!cppSubMenuHash.contains(dirName)) {
                    cppSubMenu = new QMenu(name, this);
                    cppSubMenu->setObjectName(dirName);
                    cppSubMenuHash.insert(dirName, cppSubMenu);
                } else {
                    cppSubMenu = cppSubMenuHash.value(dirName);
                }
                if (!qmlSubMenuHash.contains(dirName)) {
                    qmlSubMenu = new QMenu(name, this);
                    qmlSubMenu->setObjectName(dirName);
                    qmlSubMenuHash.insert(dirName, qmlSubMenu);
                } else {
                    qmlSubMenu = qmlSubMenuHash.value(dirName);
                }
                inExamples = true;
            }
            if (inExamples && reader.name() == QLatin1String("example")) {
                const QChar slash = QLatin1Char('/');
                const QString name = reader.attributes().value(QLatin1String("name")).toString();
                const bool isQml = reader.attributes().value(QLatin1String("qml")).toString() == "true";
                const QString localDir = reader.attributes().value(QLatin1String("filename")).toString();
                const QString fileName = localDir.section('/', -1);
                QString replacedFileName = fileName;
                replacedFileName.replace(QLatin1Char('-'), QString());
                QString relativeProPath = slash + dirName + slash + localDir + slash + replacedFileName;

                QString finalFileName = examplePath + relativeProPath + QLatin1String(".pro");

                if (!QFile::exists(finalFileName))
                    finalFileName = sourcePath + QLatin1String("/examples") + relativeProPath  + QLatin1String(".pro");

                if (isQml && !QFile::exists(finalFileName)) {
                    // maybe it's an old-style QML project?
                    relativeProPath = slash + dirName + slash + localDir + slash + fileName;
                    finalFileName = examplePath + relativeProPath + QLatin1String(".qmlproject");

                    if (!QFile::exists(finalFileName))
                        finalFileName = sourcePath + QLatin1String("/examples") + relativeProPath  + QLatin1String(".qmlproject");;
                }

                if (!QFile::exists(finalFileName))
                    break;

                QString dirNameforHelp = dirName;
                dirNameforHelp.replace(slash, QLatin1Char('-'));
                QString helpPath = QLatin1String("qthelp://com.trolltech.qt/qdoc/") +
                        dirNameforHelp +
                        QLatin1Char('-') + fileName + QLatin1String(".html");

                QAction *exampleAction = 0;
                QAction *beforeAction = 0;
                bool duplicate = false;
                QMenu *subMenu;
                subMenu = isQml ? qmlSubMenu : cppSubMenu;

                foreach (beforeAction, subMenu->actions()) {
                    int res = beforeAction->text().compare(name, Qt::CaseInsensitive);
                    if (res==0) {
                        duplicate = true;
                        break;
                    } else if (res<0)
                        beforeAction = 0;
                    else if (res>0) {
                        break;
                    }
                }

                if (!duplicate) {
                    exampleAction = new QAction(name, subMenu);
                    subMenu->insertAction(beforeAction, exampleAction);
                    connect(exampleAction, SIGNAL(triggered()), SLOT(slotOpenExample()));
                    exampleAction->setProperty(ExamplePathPropertyName, finalFileName);
                    exampleAction->setProperty(HelpPathPropertyName, helpPath);
                    if (isQml)
                        exampleAction->setProperty(QmlMainFileName, fileName);
                }
            }
            break;
        case QXmlStreamReader::EndElement:
            if (inExamples && reader.name() == QLatin1String("category")) {
                if (cppSubMenu->actions().isEmpty())
                    delete cppSubMenuHash.take(dirName);

                if (qmlSubMenu->actions().isEmpty())
                    delete qmlSubMenuHash.take(dirName);

                inExamples = false;
            }
            break;
        default:
            break;
        }
    }
}

bool menuEntryCompare(QMenu* first, QMenu* second)
{
    return (QString::localeAwareCompare(first->title(), second->title()) < 0);
}

void GettingStartedWelcomePageWidget::updateExamples(const QString &examplePath,
                                                     const QString &demosPath,
                                                     const QString &sourcePath)
{

    QString demoXml = demosPath + "/qtdemo/xml/examples.xml";
    if (!QFile::exists(demoXml)) {
        demoXml = sourcePath + "/demos/qtdemo/xml/examples.xml";
        if (!QFile::exists(demoXml))
            return;
    }

    QMenuHash cppSubMenuHash;
    QMenuHash qmlSubMenuHash;

    const QString dropDownLabel = tr("Choose an Example...");
    QMenu *cppMenu = new QMenu(ui->cppExamplesButton);
    ui->cppExamplesButton->setMenu(cppMenu);
    QMenu *qmlMenu = new QMenu(ui->qmlExamplesButton);


    // let Creator's files take precedence
    QString localQmlExamplesXml =
            Core::ICore::instance()->resourcePath()+QLatin1String("/examplebrowser/qmlexamples.xml");

    QFile localDescriptions(localQmlExamplesXml);
    if (localDescriptions.open(QFile::ReadOnly)) {
        parseXmlFile(&localDescriptions, cppSubMenuHash, qmlSubMenuHash, examplePath, sourcePath);
    }

    QFile descriptions(demoXml);
    if (!descriptions.open(QFile::ReadOnly))
        return;

    ui->cppExamplesButton->setEnabled(true);
    ui->cppExamplesButton->setText(dropDownLabel);

    parseXmlFile(&descriptions, cppSubMenuHash, qmlSubMenuHash, examplePath, sourcePath);

    QList<QMenu*> cppSubMenus = cppSubMenuHash.values();
    qSort(cppSubMenus.begin(), cppSubMenus.end(), menuEntryCompare);
    QList<QMenu*> qmlSubMenus = qmlSubMenuHash.values();
    qSort(qmlSubMenus.begin(), qmlSubMenus.end(), menuEntryCompare);

    foreach (QMenu *menu, cppSubMenus)
        cppMenu->addMenu(menu);
    foreach (QMenu *menu, qmlSubMenus)
        qmlMenu->addMenu(menu);

    if (!qmlMenu->isEmpty()) {
        ui->qmlExamplesButton->setMenu(qmlMenu);
        ui->qmlExamplesButton->setEnabled(true);
        ui->qmlExamplesButton->setText(dropDownLabel);
    }
}

namespace {
void copyRecursive(const QDir& from, const QDir& to, const QString& dir)
{
    QDir dest(to);
    dest.mkdir(dir);
    dest.cd(dir);
    QDir src(from);
    src.cd(dir);
    foreach(const QFileInfo& roFile, src.entryInfoList(QDir::Files)) {
        QFile::copy(roFile.absoluteFilePath(), dest.absolutePath() + '/' + roFile.fileName());
    }
    foreach(const QString& roDir, src.entryList(QDir::NoDotAndDotDot|QDir::Dirs)) {
        copyRecursive(src, dest, QDir(roDir).dirName());
    }
}
} // namespace

void GettingStartedWelcomePageWidget::slotOpenExample()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    QString helpFile = action->property(HelpPathPropertyName).toString();
    QString proFile = action->property(ExamplePathPropertyName).toString();
    QString qmlMainFileName;
    bool isQmlProject = false;
    if (action->dynamicPropertyNames().contains(QmlMainFileName)) {
        qmlMainFileName = action->property(QmlMainFileName).toString();
        isQmlProject = true;
    }
    QStringList files;

    QFileInfo proFileInfo(proFile);
    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    if (!proFileInfo.isWritable())
    {
        QDialog d;
        QGridLayout *lay = new QGridLayout(&d);
        QLabel *descrLbl = new QLabel;
        d.setWindowTitle(tr("Copy Project to writable Location?"));
        descrLbl->setTextFormat(Qt::RichText);
        descrLbl->setWordWrap(true);
        descrLbl->setText(tr("<p>The project you are about to open is located in the "
                             "write-protected location:</p><blockquote>%1</blockquote>"
                             "<p>Please select a writable location below and click \"Copy Project and Open\" "
                             "to open a modifiable copy of the project or click \"Keep Project and Open\" "
                             "to open the project in location.</p><p><b>Note:</b> You will not "
                             "be able to alter or compile your project in the current location.</p>")
                          .arg(QDir::toNativeSeparators(proFileInfo.dir().absolutePath())));
        lay->addWidget(descrLbl, 0, 0, 1, 2);
        QLabel *txt = new QLabel(tr("&Location:"));
        Utils::PathChooser *chooser = new Utils::PathChooser;
        txt->setBuddy(chooser);
        chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
        QSettings *settings = Core::ICore::instance()->settings();
        chooser->setPath(settings->value(
                QString::fromLatin1("General/ProjectsFallbackRoot"), QDir::homePath()).toString());
        lay->addWidget(txt, 1, 0);
        lay->addWidget(chooser, 1, 1);
        QDialogButtonBox *bb = new QDialogButtonBox;
        connect(bb, SIGNAL(accepted()), &d, SLOT(accept()));
        connect(bb, SIGNAL(rejected()), &d, SLOT(reject()));
        QPushButton *copyBtn = bb->addButton(tr("&Copy Project and Open"), QDialogButtonBox::AcceptRole);
        copyBtn->setDefault(true);
        bb->addButton(tr("&Keep Project and Open"), QDialogButtonBox::RejectRole);
        lay->addWidget(bb, 2, 0, 1, 2);
        connect(chooser, SIGNAL(validChanged(bool)), copyBtn, SLOT(setEnabled(bool)));
        if (d.exec() == QDialog::Accepted) {
            QString exampleDirName = proFileInfo.dir().dirName();
            QString toDir = chooser->path();
            settings->setValue(QString::fromLatin1("General/ProjectsFallbackRoot"), toDir);
            QDir toDirWithExamplesDir(toDir);
            if (toDirWithExamplesDir.cd(exampleDirName)) {
                toDirWithExamplesDir.cdUp(); // step out, just to not be in the way
                QMessageBox::warning(topLevelWidget(), tr("Warning"),
                                     tr("The specified location already exists. "
                                        "Please specify a valid location."),
                                     QMessageBox::Ok, QMessageBox::NoButton);
                return;
            } else {
                QDir from = proFileInfo.dir();
                from.cdUp();
                copyRecursive(from, toDir, exampleDirName);
                // set vars to new location
                proFileInfo = QFileInfo(toDir + '/'+ exampleDirName + '/' + proFileInfo.fileName());
                proFile = proFileInfo.absoluteFilePath();
            }
        }
    }

    QString tryFile;
    if (isQmlProject) {
        tryFile = proFileInfo.path() + '/' + "/main.qml";
        if(!QFile::exists(tryFile))
            tryFile = proFileInfo.path() + "/qml/" + qmlMainFileName + ".qml";
        // legacy qmlproject case
        if(!QFile::exists(tryFile))
            tryFile = proFileInfo.path() + '/' + qmlMainFileName + ".qml";
        if(QFile::exists(tryFile))
            files << tryFile;
    } else {
        tryFile = proFileInfo.path() + "/main.cpp";
        if(!QFile::exists(tryFile))
            tryFile = proFileInfo.path() + '/' + proFileInfo.baseName() + ".cpp";
        files << tryFile;
    }
    if (ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFile)) {
        Core::ICore::instance()->openFiles(files);
        if (!helpFile.isEmpty()) {
            // queue this to make sure it gets executed after the editor widget
            // has been drawn, so we know whether to show a split help or not
            QMetaObject::invokeMethod(this, "slotOpenContextHelpPage",
                                      Qt::QueuedConnection, Q_ARG(QString, helpFile));
        }
    }
}

void GettingStartedWelcomePageWidget::slotOpenHelpPage(const QString& url)
{
    Core::HelpManager *helpManager = Core::HelpManager::instance();
    Q_ASSERT(helpManager);
    helpManager->handleHelpRequest(url);
}
void GettingStartedWelcomePageWidget::slotOpenContextHelpPage(const QString& url)
{
    Core::HelpManager *helpManager = Core::HelpManager::instance();
    Q_ASSERT(helpManager);
    helpManager->handleHelpRequest(url % QLatin1String("?view=split"));
}

void GettingStartedWelcomePageWidget::slotCreateNewProject()
{
    Core::ICore::instance()->showNewItemDialog(tr("New Project"),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}

void GettingStartedWelcomePageWidget::slotNextTip()
{
    QStringList tips = tipsOfTheDay();
    m_currentTip = ((m_currentTip+1)%tips.count());
    ui->didYouKnowTextBrowser->setText(tips.at(m_currentTip));
}

void GettingStartedWelcomePageWidget::slotPrevTip()
{
    QStringList tips = tipsOfTheDay();
    m_currentTip = ((m_currentTip-1)+tips.count())%tips.count();
    ui->didYouKnowTextBrowser->setText(tips.at(m_currentTip));
}

QStringList GettingStartedWelcomePageWidget::tipsOfTheDay()
{
    static QStringList tips;
    if (tips.isEmpty()) {
        QString altShortcut =
#ifdef Q_WS_MAC
            tr("Cmd", "Shortcut key");
#else
            tr("Alt", "Shortcut key");
#endif

        QString ctrlShortcut =
#ifdef Q_WS_MAC
            tr("Cmd", "Shortcut key");
#else
            tr("Ctrl", "Shortcut key");
#endif

        //:%1 gets replaced by Alt (Win/Unix) or Cmd (Mac)
        tips.append(tr("You can show and hide the side bar using <tt>%1+0<tt>.").arg(altShortcut));
        tips.append(tr("You can fine tune the <tt>Find</tt> function by selecting &quot;Whole Words&quot; "
                       "or &quot;Case Sensitive&quot;. Simply click on the icons on the right end of the line edit."));
        tips.append(tr("If you add external libraries to your project, Qt Creator will automatically offer syntax highlighting "
                        "and code completion."));
        tips.append(tr("The code completion is CamelCase-aware. For example, to complete <tt>namespaceUri</tt> "
                       "you can just type <tt>nU</tt> and hit <tt>Ctrl+Space</tt>."));
        tips.append(tr("You can force code completion at any time using <tt>Ctrl+Space</tt>."));
        tips.append(tr("You can start Qt Creator with a session by calling <tt>qtcreator &lt;sessionname&gt;</tt>."));
        tips.append(tr("You can return to edit mode from any other mode at any time by hitting <tt>Escape</tt>."));
        //:%1 gets replaced by Alt (Win/Unix) or Cmd (Mac)
        tips.append(tr("You can switch between the output pane by hitting <tt>%1+n</tt> where n is the number denoted "
                       "on the buttons at the window bottom: <br /><br />"
                       "1: Build Issues, 2: Search Results, 3: Application Output, "
                       "4: Compile Output").arg(altShortcut));
        tips.append(tr("You can quickly search methods, classes, help and more using the "
                       "<a href=\"qthelp://com.nokia.qtcreator/doc/creator-editor-locator.html\">Locator bar</a> (<tt>%1+K</tt>).").arg(ctrlShortcut));
        tips.append(tr("You can add custom build steps in the "
                       "<a href=\"qthelp://com.nokia.qtcreator/doc/creator-build-settings.html\">build settings</a>."));
        tips.append(tr("Within a session, you can add "
                       "<a href=\"qthelp://com.nokia.qtcreator/doc/creator-build-dependencies.html\">dependencies</a> between projects."));
        tips.append(tr("You can set the preferred editor encoding for every project in <tt>Projects -> Editor Settings -> Default Encoding</tt>."));
        tips.append(tr("You can use Qt Creator with a number of <a href=\"qthelp://com.nokia.qtcreator/doc/creator-version-control.html\">"
                       "revision control systems</a> such as Subversion, Perforce, CVS and Git."));
        tips.append(tr("In the editor, <tt>F2</tt> follows symbol definition, <tt>Shift+F2</tt> toggles declaration and definition "
                       "while <tt>F4</tt> toggles header file and source file."));
    }
    return tips;
}

void GettingStartedWelcomePageWidget::addToFeatures(const Core::RssItem &feature)
{
    m_featuredItems.append(feature);
    if (m_featuredItems.count() > 1) {
        ui->nextFeatureBtn->show();
        ui->prevFeatureBtn->show();
    }
}

void GettingStartedWelcomePageWidget::showFeature(int feature)
{
    if (m_featuredItems.isEmpty())
        return;

    if (feature == -1) {
        srand(QDateTime::currentDateTime().toTime_t());
        m_currentFeature = rand()%m_featuredItems.count();
    }

    const Core::RssItem &item = m_featuredItems.at(m_currentFeature);
    ui->featuredTextLabel->setTextFormat(Qt::RichText);
    QString text = QString::fromLatin1("<b style='color: rgb(85, 85, 85);'>%1</b><br><b>%2</b><br/><br/>%3").arg(item.category).arg(item.title).arg(item.description);
    ui->featuredTextLabel->setText(text);
    QString imagePath = item.imagePath;
    if (!imagePath.startsWith("http")) {
        imagePath = Core::ICore::instance()->resourcePath() + "/rss/" + item.imagePath;
        ui->featuredImage->setPixmap(QPixmap(imagePath));
    } else {
        new PixmapDownloader(QUrl(imagePath), ui->featuredImage);
    }

    if (item.category == QLatin1String("Event")) {
        ui->detailsLabel->setText(tr("<a href='%1'>Details...</a>").arg(item.url));
        ui->detailsLabel->show();
        ui->detailsLabel->setOpenExternalLinks(true);
    }
    else if (item.category == QLatin1String("Tutorial")) {
        ui->detailsLabel->setText(tr("<a href='%1'>Take Tutorial</a>").arg(item.url+"?view=split"));
        ui->detailsLabel->show();
        ui->detailsLabel->setOpenExternalLinks(true);
    }
}

void GettingStartedWelcomePageWidget::slotNextFeature()
{
    QTC_ASSERT(!m_featuredItems.isEmpty(), return);
    m_currentFeature = (m_currentFeature+1) % m_featuredItems.count();
    showFeature(m_currentFeature);
}

void GettingStartedWelcomePageWidget::slotPrevFeature()
{
    QTC_ASSERT(!m_featuredItems.isEmpty(), return);
    m_currentFeature = ((m_currentFeature-1)+m_featuredItems.count()) % m_featuredItems.count();
    showFeature(m_currentFeature);
}

} // namespace Internal
} // namespace Qt4ProjectManager
