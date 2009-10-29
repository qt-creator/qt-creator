/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <coreplugin/coreconstants.h>

#include <utils/pathchooser.h>

#include <extensionsystem/pluginmanager.h>

#include <help/helpplugin.h>

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QSettings>
#include <QtCore/QXmlStreamReader>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFont>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

namespace Qt4ProjectManager {
namespace Internal {

GettingStartedWelcomePageWidget::GettingStartedWelcomePageWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GettingStartedWelcomePageWidget)
{
    ui->setupUi(this);
    ui->tutorialsTitleLabel->setStyledText(tr("Tutorials"));
    ui->demoTitleLabel->setStyledText(tr("Explore Qt Examples"));
    ui->didYouKnowTextBrowser->viewport()->setAutoFillBackground(false);
    ui->didYouKnowTitleLabel->setStyledText(tr("Did You Know?"));

    connect(ui->tutorialTreeWidget, SIGNAL(activated(QString)), SLOT(slotOpenHelpPage(const QString&)));
    connect(ui->openExampleButton, SIGNAL(clicked()), SLOT(slotOpenExample()));
    connect(ui->examplesComboBox, SIGNAL(currentIndexChanged(int)), SLOT(slotEnableExampleButton(int)));

    ui->tutorialTreeWidget->addItem(tr("<b>Qt Creator - A quick tour</b>"),
                                        QString("qthelp://com.nokia.qtcreator.%1%2/doc/index.html").arg(IDE_VERSION_MAJOR).arg(IDE_VERSION_MINOR));
    ui->tutorialTreeWidget->addItem(tr("Creating an address book"),
                                        QLatin1String("qthelp://com.nokia.qtcreator/doc/tutorials-addressbook-sdk.html?view=split"));
    ui->tutorialTreeWidget->addItem(tr("Understanding widgets"),
                                        QLatin1String("qthelp://com.trolltech.qt/qdoc/widgets-tutorial.html?view=split"));
    ui->tutorialTreeWidget->addItem(tr("Building with qmake"),
                                        QLatin1String("qthelp://com.trolltech.qmake/qdoc/qmake-tutorial.html?view=split"));
    ui->tutorialTreeWidget->addItem(tr("Writing test cases"),
                                        QLatin1String("qthelp://com.trolltech.qt/qdoc/qtestlib-tutorial.html?view=split"));

    srand(QDateTime::currentDateTime().toTime_t());
    QStringList tips = tipsOfTheDay();
    m_currentTip = rand()%tips.count();

    QTextDocument *doc = ui->didYouKnowTextBrowser->document();
    doc->setDefaultStyleSheet("a:link {color:black;}");
    ui->didYouKnowTextBrowser->setDocument(doc);
    ui->didYouKnowTextBrowser->setText(tips.at(m_currentTip));

    connect(ui->nextTipBtn, SIGNAL(clicked()), this, SLOT(slotNextTip()));
    connect(ui->prevTipBtn, SIGNAL(clicked()), this, SLOT(slotPrevTip()));

}

GettingStartedWelcomePageWidget::~GettingStartedWelcomePageWidget()
{
    delete ui;
}

void GettingStartedWelcomePageWidget::updateExamples(const QString& examplePath, const QString& demosPath, const QString &sourcePath)
{
    QString demoxml = demosPath + "/qtdemo/xml/examples.xml";
    if (!QFile::exists(demoxml)) {
        demoxml = sourcePath + "/demos/qtdemo/xml/examples.xml";
        if (!QFile::exists(demoxml))
            return;
    }

    QFile description(demoxml);
    if (!description.open(QFile::ReadOnly))
        return;

    ui->examplesComboBox->clear();
    ui->examplesComboBox->setEnabled(true);

    ui->examplesComboBox->addItem(tr("Choose an example..."));
    QFont f = font();
    f.setItalic(true);
    ui->examplesComboBox->setItemData(0, f, Qt::FontRole);
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
                ui->examplesComboBox->addItem(name);
                f.setBold(true);
                ui->examplesComboBox->setItemData(ui->examplesComboBox->count()-1, f, Qt::FontRole);
                f.setBold(false);
                inExamples = true;
            }
            if (inExamples && reader.name() == "example") {
                QString name = reader.attributes().value(QLatin1String("name")).toString();
                QString fn = reader.attributes().value(QLatin1String("filename")).toString();
                QString relativeProPath = '/' + dirName + '/' + fn + '/' + fn + ".pro";
                QString fileName = examplePath + relativeProPath;
                if (!QFile::exists(fileName))
                    fileName = sourcePath + "/examples" + relativeProPath;
                QString helpPath = "qthelp://com.trolltech.qt/qdoc/" + dirName.replace("/", "-") + "-" + fn + ".html";

                ui->examplesComboBox->addItem("  " + name, fileName);
                ui->examplesComboBox->setItemData(ui->examplesComboBox->count()-1, helpPath, Qt::UserRole+1);
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

void GettingStartedWelcomePageWidget::slotEnableExampleButton(int index)
{
    QString fileName = ui->examplesComboBox->itemData(index, Qt::UserRole).toString();
    ui->openExampleButton->setEnabled(!fileName.isEmpty());
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
    QComboBox *box = ui->examplesComboBox;
    QString proFile = box->itemData(box->currentIndex(), Qt::UserRole).toString();
    QString helpFile = box->itemData(box->currentIndex(), Qt::UserRole + 1).toString();
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
    if(QFile::exists(tryFile))
        files << tryFile;
    Core::ICore::instance()->openFiles(files);
    slotOpenContextHelpPage(helpFile);
}

void GettingStartedWelcomePageWidget::slotOpenHelpPage(const QString& url)
{
    Help::HelpManager *helpManager
        = ExtensionSystem::PluginManager::instance()->getObject<Help::HelpManager>();
    Q_ASSERT(helpManager);
    helpManager->openHelpPage(url);
}
void GettingStartedWelcomePageWidget::slotOpenContextHelpPage(const QString& url)
{
    Help::HelpManager *helpManager
        = ExtensionSystem::PluginManager::instance()->getObject<Help::HelpManager>();
    Q_ASSERT(helpManager);
    helpManager->openContextHelpPage(url);
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


        tips.append(tr("You can switch between Qt Creator's modes using <tt>Ctrl+number</tt>:<ul>"
                       "<li>1 - Welcome</li><li>2 - Edit</li><li>3 - Debug</li><li>4 - Projects</li><li>5 - Help</li>"
                       "<li></li><li>6 - Output</li></ul>"));
        //:%1 gets replaced by Alt (Win/Unix) or Cmd (Mac)
        tips.append(tr("You can show and hide the side bar using <tt>%1+0<tt>.").arg(altShortcut));
        tips.append(tr("You can fine tune the <tt>Find</tt> function by selecting &quot;Whole Words&quot; "
                       "or &quot;Case Sensitive&quot;. Simply click on the icons on the right end of the line edit."));
        tips.append(tr("If you add <a href=\"qthelp://com.nokia.qtcreator/doc/creator-external-library-handling.html\""
                       ">external libraries</a>, Qt Creator will automatically offer syntax highlighting "
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
                       "<a href=\"qthelp://com.nokia.qtcreator/doc/creator-build-settings.html#dependencies\">dependencies</a> between projects."));
        tips.append(tr("You can set the preferred editor encoding for every project in <tt>Projects -> Editor Settings -> Default Encoding</tt>."));
        tips.append(tr("You can modify the binary that is being executed when you press the <tt>Run</tt> button: Add a <tt>Custom Executable</tt> "
                       "by clicking the <tt>+</tt> button in <tt>Projects -> Run Settings -> Run Configuration</tt> and then select the new "
                       "target in the combo box."));
        tips.append(tr("You can use Qt Creator with a number of <a href=\"qthelp://com.nokia.qtcreator/doc/creator-version-control.html\">"
                       "revision control systems</a> such as Subversion, Perforce, CVS and Git."));
        tips.append(tr("In the editor, <tt>F2</tt> follows symbol definition, <tt>Shift+F2</tt> toggles declaration and definition "
                       "while <tt>F4</tt> toggles header file and source file."));
    }
    return tips;
}


} // namespace Internal
} // namespace Qt4ProjectManager
