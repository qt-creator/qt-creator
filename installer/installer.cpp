/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

// This file contains the QtCreator-specific part of the installer.
// It lists the files and features the installer should handle.

#include "qinstaller.h"
#include "qinstallergui.h"

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>

#include <QtGui/QApplication>
#include <QtGui/QBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>


// QInstallerGui is base of the Gui part of an installer, i.e.
// the "main installer wizard". In the simplest case it's just
// a sequence of "standard" wizard pages. A few commonly used
// ones are provided already in qinstallergui.h.


// A custom target directory selection based due to the no-space
// restriction...

class TargetDirectoryPage : public QInstallerTargetDirectoryPage
{
public:
    TargetDirectoryPage(QInstaller *installer)
      : QInstallerTargetDirectoryPage(installer)
    {
        m_noSpaceLabel = new QLabel(this);
        m_noSpaceLabel->setText("The directory name should not contain any space.");
        m_noSpaceLabel->hide();
        insertWidget(m_noSpaceLabel, "MessageLabel");
    }

    bool isComplete() const
    {
        bool invalid = targetDir().contains(' ');
        QPalette palette;
        // We show the warning only if the user types a space.
        // No need to scare him if the path is ok for us...
        if (invalid) {
            m_noSpaceLabel->show();
            palette.setColor(QPalette::WindowText, Qt::red);
        }
        m_noSpaceLabel->setPalette(palette);
        return !invalid;
    }
    
    int nextId() const
    {
        QFileInfo fi(targetDir());
    
        if (isVisible() && fi.isDir()) {
            QFileInfo fi2(targetDir() + '/' + installer()->uninstallerName());
            if (fi2.exists()) {
                QMessageBox::StandardButton bt = QMessageBox::warning(wizard(),
                    tr("Warning"),
                    tr("The directory you selected exists already and contains an installaion.\n"
                       "Do you want to overwrite it?"),
                        QMessageBox::Yes | QMessageBox::No);
                if (bt == QMessageBox::No)
                    return wizard()->currentId();
            } else {
                QMessageBox::StandardButton bt = QMessageBox::warning(wizard(),
                    tr("Warning"),
                    tr("The directory you selected exists already.\n"
                       "Do you want to remove it and continue?"),
                        QMessageBox::Yes | QMessageBox::No);
                if (bt == QMessageBox::No)
                    return wizard()->currentId();
            }
        }
        return QInstallerPage::nextId();
    }

private:
    QLabel *m_noSpaceLabel;
};


////////////////////////////////////////////////////////////////////
//
// QtCreatorInstallerGui
//
////////////////////////////////////////////////////////////////////

// QtCreatorInstallerGui is the QtCreator specific incarnation
// of a QInstallerGui.

class QtCreatorInstallerGui : public QInstallerGui
{
public:
    QtCreatorInstallerGui(QInstaller *installer)
    {
        // The Gui has access to the installer backend at construction
        // time. For later access it needs to store the QInstaller *
        // pointer provided. Not needed in this case here.

        setWindowTitle(tr("%1 Setup").arg(installer->value("ProductName")));
        installer->connectGui(this);

        // We are happy with the default set of pages.
        addPage(new QInstallerIntroductionPage(installer));
        addPage(new QInstallerLicenseAgreementPage(installer));
        //addPage(new QInstallerTargetDirectoryPage(installer));
        addPage(new TargetDirectoryPage(installer));
        if (installer->componentCount() > 1)
            addPage(new QInstallerComponentSelectionPage(installer));
        addPage(new QInstallerReadyForInstallationPage(installer));
        addPage(new QInstallerPerformInstallationPage(installer));
        addPage(new QInstallerFinishedPage(installer));

        setStartId(installer->value("GuiStartPage").toInt());
    }
};


// QInstaller is base of the "backend" part of an installer, i.e.
// the part handling the installer tasks and keeping track of 
// related data like the directory to install to, the name of
// the Product, version of the Product etc.

// QtCreatorInstaller is the QtCreator specific incarnation
// of a QInstaller. It needs to list all tasks that a performed
// during installation. The tasks themselves specify what to 
// do at uninstall time.

class QtCreatorInstaller : public QInstaller
{
public:
    QtCreatorInstaller()
    {
        // basic product information
        setValue("ProductName", "Qt Creator");
        setValue("ProductVersion", "alpha");

        // registration information
        setValue("Comments", "");
        setValue("Contact", "");
        setValue("DisplayVersion", "");
        setValue("HelpLink", "");
        setValue("Publisher", "");
        setValue("UrlAboutInfo", "");

        // information needed at installer generation time
        setValue("OutputFile", "qtcreator-installer");
        setValue("RunProgram", "@TargetDir@/bin/qtcreator");

        // default component selection, overridable from command line
        setValue("UseQtCreator", "true");
#ifdef Q_OS_WIN
        //setValue("UseQt", "true");
        //setValue("UseMinGW", "true");
#endif
    }
    
private:
    // tasks related to QtCreator itself. Binary, libraries etc.
    void appendQtCreatorComponent()
    {
        QString sourceDir = value("SourceDir");
        if (sourceDir.isEmpty() && QFileInfo("../bin/qtcreator").isReadable())
            sourceDir = QLatin1String("..");
        if (sourceDir.isEmpty())
            throw QInstallerError("Missing 'SourceDir=<dir>' on command line.");
        QInstallerComponent *component = new QInstallerComponent(this);
        component->setValue("Name", "QtCreator");
        component->setValue("DisplayName", "Qt Creator");
        component->setValue("Description", "The Qt Creator IDE");
        component->setValue("SuggestedState", "AlwaysInstalled");
#ifdef Q_OS_MAC
        component->appendDirectoryTasks(sourceDir + "/bin/QtCreator.app", "@TargetDir@");
#else
        component->appendDirectoryTasks(sourceDir + "/bin", "@TargetDir@/bin");
        component->appendDirectoryTasks(sourceDir + "/lib", "@TargetDir@/lib");
#endif

        QInstallerPatchFileTask *task = new QInstallerPatchFileTask(this);
        task->setTargetPath("@TargetDir@/lib/Trolltech/" + libraryName("ProjectExplorer", "1.0.0"));
        task->setNeedle("Clear Session");
        task->setReplacement("CLEAR SESSION");
        component->appendTask(task);

        appendComponent(component);
    }

    void appendMinGWComponent()
    {
        QString mingwSourceDir = value("MinGWSourceDir");
        if (mingwSourceDir.isEmpty())
            throw QInstallerError("Missing 'MinGWSourceDir=<dir>' on command line.");
        QInstallerComponent *component = new QInstallerComponent(this);
        component->setValue("Name", "MinGW");
        component->setValue("DisplayName", "MinGW");
        component->setValue("Description",
            "The MinGW environment including the g++ compiler "
            "and the gdb debugger.");
        component->setValue("SuggestedState", "Installed");
        component->appendDirectoryTasks(mingwSourceDir, "@TargetDir@");
        appendComponent(component);
    }

    void appendQtComponent()
    {
        QString qtSourceDir = value("QtSourceDir");
        if (qtSourceDir.isEmpty())
            throw QInstallerError("Missing 'QtSourceDir=<dir>' on command line.");

        QInstallerComponent *component = new QInstallerComponent(this);
        component->setValue("Name", "Qt Development Libraries");
        component->setValue("DisplayName", "Qt");
        component->setValue("Description",
            "The Qt library files for development including "
            "documentation");
        component->setValue("SuggestedState", "Installed");
        component->appendDirectoryTasks(qtSourceDir, "@TargetDir@");

#ifdef Q_OS_WIN
        static const struct
        {
            const char *fileName;
            const char *sourceLocation;
        } libs[] = {
            {"/bin/Qt3Support", "/src/qt3support/"},
            {"/bin/QtCore", "/src/corelib/"},
            {"/bin/QtGui", "/src/gui/"},
            {"/bin/QtHelp", "/tools/assistant/lib/"},
            {"/bin/QtNetwork", "/src/network/"},
            {"/bin/QtOpenGL", "/src/opengl/"},
            {"/bin/QtScript", "/src/script/"},
            {"/bin/QtSql", "/src/sql/"},
            {"/bin/QtSvg", "/src/svg/"},
            {"/bin/QtTest", "/src/testlib/"},
            {"/bin/QtWebKit", "/src/3rdparty/webkit/WebCore/"},
            {"/bin/QtXml", "/src/xml/"},
            {"/bin/QtXmlPatterns", "/src/xmlpatterns/"},
            {"/plugins/accessible/qtaccessiblecompatwidgets",
                "/src/plugins/accessible/compat/"},
            {"/plugins/accessible/qtaccessiblewidgets",
                "/src/plugins/accessible/widgets/"},
            {"/plugins/codecs/qcncodecs", "/src/plugins/codecs/cn/"},
            {"/plugins/codecs/qjpcodecs", "/src/plugins/codecs/jp/"},
            {"/plugins/codecs/qkrcodecs", "/src/plugins/codecs/kr/"},
            {"/plugins/codecs/qtwcodecs", "/src/plugins/codecs/tw/"},
            {"/plugins/iconengines/qsvgicon", "/src/plugins/iconengines/svgiconengine/"},
            {"/plugins/imageformats/qgif", "/src/plugins/imageformats/gif/"},
            {"/plugins/imageformats/qjpeg", "/src/plugins/imageformats/jpeg/"},
            {"/plugins/imageformats/qmng", "/src/plugins/imageformats/mng/"},
            {"/plugins/imageformats/qsvg", "/src/plugins/imageformats/svg/"},
            {"/plugins/imageformats/qtiff", "/src/plugins/imageformats/tiff/"},
            {"/plugins/sqldrivers/qsqlite", "/src/plugins/sqldrivers/sqlite/"},
    //        {"/plugins/sqldrivers/qsqlodbc", "/src/plugins/sqldrivers/odbc/"}
        };
        
        QString debug = QLatin1String("d4.dll");

        for (int i = 0; i != sizeof(libs) / sizeof(libs[0]); ++i) {
            QInstallerPatchFileTask *task = new QInstallerPatchFileTask(this);
            task->setTargetPath(QString("@TargetDir@/") + libs[i].fileName + debug);
            task->setNeedle("f:/depot/qt");
            task->setReplacement(QByteArray("@TargetDir@/") + libs[i].sourceLocation);
            component->appendTask(task);
        }
#endif

        appendComponent(component);
    }

    void createTasks()
    {
        // set UseXXX=false on command line to prevent inclusion of the 
        // respective component
        if (value("UseQtCreator") == "true")
            appendQtCreatorComponent();
        if (value("UseMinGW") == "true")
            appendMinGWComponent();
        if (value("UseQt") == "true")
            appendQtComponent();
    }
};


////////////////////////////////////////////////////////////////////
//
// QtCreatorUninstallerGui
//
////////////////////////////////////////////////////////////////////

class QtCreatorUninstallerGui : public QObject
{
public:
    QtCreatorUninstallerGui(QInstaller *installer)
      : m_installer(installer)
    {}

    int exec()
    {
        QMessageBox::StandardButton bt = QMessageBox::question(0,
            tr("Question"),
            tr("Do you want to deinstall %1 and all of its modules?")
                .arg(m_installer->value("ProductName")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (bt == QMessageBox::No)
            return 0;
        QWizard wizard;
        wizard.addPage(new QInstallerPerformUninstallationPage(m_installer));
        wizard.show();  
        return qApp->exec();
    }

private:
    QInstaller *m_installer;
};



////////////////////////////////////////////////////////////////////
//
// The Main Driver Program
//
////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList args = app.arguments();
    qDebug() << "ARGS: " << args;

    QtCreatorInstaller installer;
    
    bool helpRequested = false;
    bool guiRequested = true;

    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).contains('=')) {
            const QString arg = args.at(i);
            const QString name = arg.section('=', 0, 0);
            const QString value = arg.section('=', 1, 1);
            installer.setValue(name, value);
        } else if (args.at(i) == "--help" || args.at(i) == "-h") {
            helpRequested = true;
        } else if (args.at(i) == "--no-gui" || args.at(i) == "NoGui") {
            qDebug() << "NOGUI";
            guiRequested = false;
        } else if (args.at(i) == "--verbose" || args.at(i) == "Verbose") {
            installer.setVerbose(true);
        } else {
            helpRequested = true;
        }
    }
    
    if (installer.isVerbose())
        installer.dump(); 

    if (helpRequested) {
        QString productName = installer.value("ProductName");
        QString str;
        if (installer.isCreator())
            str = QString("  [SourceDir=<dir>]\n"
                "\n      Creates the %1 installer.\n").arg(productName);
        else if (installer.isInstaller())
            str = QString("  [--no-gui] [<name>=<value>...]\n"
                "\n      Runs the %1 installer\n"
                "\n      If the '--no-gui' parameter is given, it runs "
                " installer without GUI\n").arg(productName);
        else if (installer.isUninstaller())
            str = QString("  [<name>=<value>...]\n"
                "\n      Runs the %1 uninstaller.\n").arg(productName);
        str = "\nUsage: " + installer.installerBinaryPath() + str;
        qDebug() << qPrintable(str);
        return 0;
    }

    if (installer.isInstaller() && guiRequested) {
        QtCreatorInstallerGui gui(&installer);
        gui.show();
        return app.exec();
    }

#ifdef Q_OS_WIN
    if (installer.isUninstaller()) {
        QStringList newArgs = args;
        newArgs.removeFirst();
        installer.restartTempUninstaller(newArgs);
        return 0;
    }
#endif
    if ((installer.isUninstaller() || installer.isTempUninstaller())
        && guiRequested) {
        QtCreatorUninstallerGui gui(&installer);
        //gui.show();
        return gui.exec();
    }

    return installer.run() ? 0 : 1;
}
