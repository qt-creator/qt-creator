/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gettingstartedwelcomepagewidget.h"
#include "ui_gettingstartedwelcomepagewidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mainwindow.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/pathchooser.h>

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
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFont>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>

namespace Qt4ProjectManager {
namespace Internal {

const char ExamplePathPropertyName[] = "__qt_ExamplePath";
const char HelpPathPropertyName[] = "__qt_HelpPath";

GettingStartedWelcomePageWidget::GettingStartedWelcomePageWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GettingStartedWelcomePageWidget)
{
    ui->setupUi(this);

#ifndef QTCREATOR_WITH_QML
    ui->demosExamplesFrame_2->hide();
#endif

    ui->didYouKnowTextBrowser->viewport()->setAutoFillBackground(false);

    connect(ui->tutorialTreeWidget, SIGNAL(activated(QString)), SLOT(slotOpenHelpPage(const QString&)));

    ui->tutorialTreeWidget->addItem(tr("The Qt Creator User Interface"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-quick-tour.html"));
    ui->tutorialTreeWidget->addItem(tr("Building and Running an Example"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-build-example-application.html?view=split"));
    ui->tutorialTreeWidget->addItem(tr("Creating a Qt C++ Application"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-writing-program.html?view=split"));
    ui->tutorialTreeWidget->addItem(tr("Creating a Mobile Application"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-mobile-example.html?view=split"));
#ifdef QTCREATOR_WITH_QML
    ui->tutorialTreeWidget->addItem(tr("Creating a Qt Quick Application"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-qml-application.html?view=split"));
#endif

    srand(QDateTime::currentDateTime().toTime_t());
    QStringList tips = tipsOfTheDay();
    m_currentTip = rand()%tips.count();

    QTextDocument *doc = ui->didYouKnowTextBrowser->document();
    doc->setDefaultStyleSheet("a:link {color:black;}");
    ui->didYouKnowTextBrowser->setDocument(doc);
    ui->didYouKnowTextBrowser->setText(tips.at(m_currentTip));

    connect(ui->nextTipBtn, SIGNAL(clicked()), this, SLOT(slotNextTip()));
    connect(ui->prevTipBtn, SIGNAL(clicked()), this, SLOT(slotPrevTip()));
    connect(ui->openProjectButton, SIGNAL(clicked()),
            ProjectExplorer::ProjectExplorerPlugin::instance(),
            SLOT(openOpenProjectDialog()));
    connect(ui->createNewProjectButton, SIGNAL(clicked()), this, SLOT(slotCreateNewProject()));

    ui->createNewProjectButton->setIcon(
            QIcon::fromTheme("document-new", ui->createNewProjectButton->icon()));
    ui->openProjectButton->setIcon(
            QIcon::fromTheme("document-open", ui->openProjectButton->icon()));
    QTimer::singleShot(0, this, SLOT(slotSetPrivateQmlExamples()));
}

GettingStartedWelcomePageWidget::~GettingStartedWelcomePageWidget()
{
    delete ui;
}

void GettingStartedWelcomePageWidget::slotSetPrivateQmlExamples()
{
    if (!ui->qmlExamplesButton->menu()) {
        const QString resPath = Core::ICore::instance()->resourcePath();
        updateQmlExamples(resPath, resPath);
    }
 }

void GettingStartedWelcomePageWidget::updateCppExamples(const QString &examplePath,
                                                        const QString &sourcePath,
                                                        const QString &demoXml)
{

    QFile description(demoXml);
    if (!description.open(QFile::ReadOnly))
        return;

    ui->cppExamplesButton->setEnabled(true);;
    ui->cppExamplesButton->setText(tr("Choose an example..."));

    QMenu *menu = new QMenu(ui->cppExamplesButton);
    ui->cppExamplesButton->setMenu(menu);

    QMenu *subMenu = 0;
    bool inExamples = false;
    QString dirName;
    QXmlStreamReader reader(&description);

    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
            if (reader.name() == QLatin1String("category")) {
                QString name = reader.attributes().value(QLatin1String("name")).toString();
                if (name.contains(QLatin1String("tutorial")))
                    break;
                dirName = reader.attributes().value(QLatin1String("dirname")).toString();
                subMenu = menu->addMenu(name);
                inExamples = true;
            }
            if (inExamples && reader.name() == QLatin1String("example")) {
                const QChar slash = QLatin1Char('/');
                const QString name = reader.attributes().value(QLatin1String("name")).toString();
                const QString fn = reader.attributes().value(QLatin1String("filename")).toString();
                const QString relativeProPath = slash + dirName + slash + fn + slash + fn + QLatin1String(".pro");
                QString fileName = examplePath + relativeProPath;
                if (!QFile::exists(fileName))
                    fileName = sourcePath + QLatin1String("/examples") + relativeProPath;
                if (!QFile::exists(fileName)) {
                    continue; // might be .qmlproject
                }

                QString dirName1 = dirName;
                dirName1.replace(slash, QLatin1Char('-'));
                QString helpPath = QLatin1String("qthelp://com.trolltech.qt/qdoc/") +
                                   dirName1 +
                                   QLatin1Char('-') + fn + QLatin1String(".html");

                QAction *exampleAction = subMenu->addAction(name);
                connect(exampleAction, SIGNAL(triggered()), SLOT(slotOpenExample()));

                exampleAction->setProperty(ExamplePathPropertyName, fileName);
                exampleAction->setProperty(HelpPathPropertyName, helpPath);
            }
            break;
            case QXmlStreamReader::EndElement:
            if (reader.name() == QLatin1String("category"))
                inExamples = false;
            break;
            default:
            break;
        }
    }

    // Remove empty categories
    foreach (QAction *action, menu->actions()) {
        if (QMenu *subMenu = action->menu()) {
            if (subMenu->isEmpty()) {
                menu->removeAction(action);
            }
        }
    }
}

void GettingStartedWelcomePageWidget::updateQmlExamples(const QString &examplePath,
                                                        const QString &sourcePath)
{
    ui->qmlExamplesButton->setText(tr("Choose an example..."));

    QStringList roots;
    roots << (examplePath + QLatin1String("/declarative"))
            << (sourcePath + QLatin1String("/examples/declarative"));
    QMap<QString, QString> exampleProjects;

    foreach (const QString &root, roots) {
        QList<QFileInfo> examples = QDir(root).entryInfoList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name);
        foreach(const QFileInfo &example, examples) {
            const QString fileName = example.fileName();
            if (exampleProjects.contains(fileName))
                continue;
            const QString exampleProject = example.absoluteFilePath()
                                           + QLatin1Char('/') + fileName
                                           + QLatin1String(".qmlproject");
            if (!QFile::exists(exampleProject))
                continue;
            exampleProjects.insert(fileName, exampleProject);
        }
    }

    if (!exampleProjects.isEmpty()) {
        QMenu *menu = new QMenu(ui->qmlExamplesButton);
        ui->qmlExamplesButton->setMenu(menu);

        QMapIterator<QString, QString> it(exampleProjects);
        while (it.hasNext()) {
            it.next();
            QAction *exampleAction = menu->addAction(it.key());
            connect(exampleAction, SIGNAL(triggered()), SLOT(slotOpenExample()));
            exampleAction->setProperty(ExamplePathPropertyName, it.value());
            // FIXME once we have help for QML examples
            // exampleAction->setProperty(HelpPathPropertyName, helpPath);
        }
    }

    ui->qmlExamplesButton->setEnabled(!exampleProjects.isEmpty());
}

void GettingStartedWelcomePageWidget::updateExamples(const QString &examplePath,
                                                     const QString &demosPath,
                                                     const QString &sourcePath)
{
    QString demoxml = demosPath + "/qtdemo/xml/examples.xml";
    if (!QFile::exists(demoxml)) {
        demoxml = sourcePath + "/demos/qtdemo/xml/examples.xml";
        if (!QFile::exists(demoxml))
            return;
    }
    updateCppExamples(examplePath, sourcePath, demoxml);
    updateQmlExamples(examplePath, sourcePath);
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
        chooser->setExpectedKind(Utils::PathChooser::Directory);
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


    QString tryFile = proFileInfo.path() + "/main.cpp";
    files << proFile;
    if(!QFile::exists(tryFile))
        tryFile = proFileInfo.path() + '/' + proFileInfo.baseName() + ".cpp";
    // maybe it's a QML project?
    if(!QFile::exists(tryFile))
        tryFile = proFileInfo.path() + '/' + "/main.qml";
    if(!QFile::exists(tryFile))
        tryFile = proFileInfo.path() + '/' + proFileInfo.baseName() + ".qml";
    if(QFile::exists(tryFile))
        files << tryFile;
    Core::ICore::instance()->openFiles(files);
    if (!helpFile.isEmpty())
        slotOpenContextHelpPage(helpFile);
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
                       "on the buttons at the window bottom:"
                       "<ul><li>1 - Build Issues</li><li>2 - Search Results</li><li>3 - Application Output</li>"
                       "<li>4 - Compile Output</li></ul>").arg(altShortcut));
        tips.append(tr("You can quickly search methods, classes, help and more using the "
                       "<a href=\"qthelp://com.nokia.qtcreator/doc/creator-navigation.html\">Locator bar</a> (<tt>%1+K</tt>).").arg(ctrlShortcut));
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


} // namespace Internal
} // namespace Qt4ProjectManager
