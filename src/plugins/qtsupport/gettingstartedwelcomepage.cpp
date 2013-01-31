/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gettingstartedwelcomepage.h"

#include "exampleslistmodel.h"
#include "screenshotcropper.h"

#include "qtsupportconstants.h"

#include <utils/pathchooser.h>
#include <utils/fileutils.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QMutex>
#include <QThread>
#include <QMutexLocker>
#include <QPointer>
#include <QWaitCondition>
#include <QDir>
#include <QBuffer>
#include <QGraphicsProxyWidget>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QImage>
#include <QImageReader>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QDeclarativeImageProvider>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDesktopServices>

using namespace Utils;

namespace QtSupport {
namespace Internal {

const char C_FALLBACK_ROOT[] = "ProjectsFallbackRoot";

QPointer<ExamplesListModel> &examplesModelStatic()
{
    static QPointer<ExamplesListModel> s_examplesModel;
    return s_examplesModel;
}

class Fetcher : public QObject
{
    Q_OBJECT

public:
    Fetcher() : QObject(),  m_shutdown(false)
    {
        connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(shutdown()));
    }

    void wait()
    {
        if (QThread::currentThread() == QApplication::instance()->thread())
            return;
        if (m_shutdown)
            return;

        m_waitcondition.wait(&m_mutex, 4000);
    }

    QByteArray data()
    {
        QMutexLocker lock(&m_dataMutex);
        return m_fetchedData;
    }

    void clearData()
    {
        QMutexLocker lock(&m_dataMutex);
        m_fetchedData.clear();
    }

    bool asynchronousFetchData(const QUrl &url)
    {
        QMutexLocker lock(&m_mutex);

        if (!QMetaObject::invokeMethod(this,
                                       "fetchData",
                                       Qt::AutoConnection,
                                       Q_ARG(QUrl, url))) {
            return false;
        }

        wait();
        return true;
    }


public slots:
    void fetchData(const QUrl &url)
    {
        if (m_shutdown)
            return;

        QMutexLocker lock(&m_mutex);

        if (Core::HelpManager::instance()) {
            QMutexLocker dataLock(&m_dataMutex);
            m_fetchedData = Core::HelpManager::instance()->fileData(url);
        }
        m_waitcondition.wakeAll();
    }

private slots:
    void shutdown()
    {
        m_shutdown = true;
    }

public:
    QByteArray m_fetchedData;
    QWaitCondition m_waitcondition;
    QMutex m_mutex;     //This mutex synchronises the wait() and wakeAll() on the wait condition.
                        //We have to ensure that wakeAll() is called always after wait().

    QMutex m_dataMutex; //This mutex synchronises the access of m_fectedData.
                        //If the wait condition timeouts we otherwise get a race condition.
    bool m_shutdown;
};

class HelpImageProvider : public QDeclarativeImageProvider
{
public:
    HelpImageProvider()
        : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
    {
    }

    // gets called by declarative in separate thread
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
        Q_UNUSED(size)
        QMutexLocker lock(&m_mutex);

        QUrl url = QUrl::fromEncoded(id.toLatin1());

        if (!m_fetcher.asynchronousFetchData(url))
            return QImage();

        if (m_fetcher.data().isEmpty())
            return QImage();
        QByteArray data = m_fetcher.data();
        QBuffer imgBuffer(&data);
        imgBuffer.open(QIODevice::ReadOnly);
        QImageReader reader(&imgBuffer);
        QImage img = reader.read();

        m_fetcher.clearData();
        return ScreenshotCropper::croppedImage(img, id, requestedSize);
    }
private:
    Fetcher m_fetcher;
    QMutex m_mutex;
};

GettingStartedWelcomePage::GettingStartedWelcomePage() : m_engine(0)
{

}

QUrl GettingStartedWelcomePage::pageLocation() const
{
    QString resourcePath = Core::ICore::resourcePath();
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    resourcePath = Utils::normalizePathName(resourcePath);
#endif

    return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/gettingstarted.qml"));
}

QString GettingStartedWelcomePage::title() const
{
    return tr("Getting Started");
}

int GettingStartedWelcomePage::priority() const
{
    return 4;
}

void GettingStartedWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    m_engine = engine;
}

GettingStartedWelcomePage::Id GettingStartedWelcomePage::id() const
{
    return GettingStarted;
}

ExamplesWelcomePage::ExamplesWelcomePage()
    : m_engine(0),  m_showExamples(false)
{
}

void ExamplesWelcomePage::setShowExamples(bool showExamples)
{
    m_showExamples = showExamples;
}

QString ExamplesWelcomePage::title() const
{
    if (m_showExamples)
        return tr("Examples");
    else
        return tr("Tutorials");
}

 int ExamplesWelcomePage::priority() const
 {
     if (m_showExamples)
         return 30;
     else
         return 40;
 }

 bool ExamplesWelcomePage::hasSearchBar() const
 {
     if (m_showExamples)
         return true;
     else
         return false;
 }

QUrl ExamplesWelcomePage::pageLocation() const
{
    QString resourcePath = Core::ICore::resourcePath();
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    resourcePath = Utils::normalizePathName(resourcePath);
#endif
    if (m_showExamples)
        return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/examples.qml"));
    else
        return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/tutorials.qml"));
}

void ExamplesWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    m_engine = engine;
    m_engine->addImageProvider(QLatin1String("helpimage"), new HelpImageProvider);
    connect (examplesModel(), SIGNAL(tagsUpdated()), SLOT(updateTagsModel()));
    ExamplesListModelFilter *proxy = new ExamplesListModelFilter(examplesModel(), this);

    proxy->setDynamicSortFilter(true);
    proxy->sort(0);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QDeclarativeContext *rootContenxt = m_engine->rootContext();
    if (m_showExamples) {
        proxy->setShowTutorialsOnly(false);
        rootContenxt->setContextProperty(QLatin1String("examplesModel"), proxy);
    } else {
        rootContenxt->setContextProperty(QLatin1String("tutorialsModel"), proxy);
    }
    rootContenxt->setContextProperty(QLatin1String("gettingStarted"), this);
}

ExamplesWelcomePage::Id ExamplesWelcomePage::id() const
{
    return m_showExamples ? Examples : Tutorials;
}

void ExamplesWelcomePage::openSplitHelp(const QUrl &help)
{
    Core::ICore::helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
}

void ExamplesWelcomePage::openHelp(const QUrl &help)
{
    Core::ICore::helpManager()->handleHelpRequest(help.toString());
}

void ExamplesWelcomePage::openUrl(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

QStringList ExamplesWelcomePage::tagList() const
{
    return examplesModel()->tags();
}

QString ExamplesWelcomePage::copyToAlternativeLocation(const QFileInfo& proFileInfo, QStringList &filesToOpen, const QStringList& dependencies)
{
    const QString projectDir = proFileInfo.canonicalPath();
    QDialog d(Core::ICore::mainWindow());
    QGridLayout *lay = new QGridLayout(&d);
    QLabel *descrLbl = new QLabel;
    d.setWindowTitle(tr("Copy Project to writable Location?"));
    descrLbl->setTextFormat(Qt::RichText);
    descrLbl->setWordWrap(false);
    const QString nativeProjectDir = QDir::toNativeSeparators(projectDir);
    descrLbl->setText(QString::fromLatin1("<blockquote>%1</blockquote>").arg(nativeProjectDir));
    descrLbl->setMinimumWidth(descrLbl->sizeHint().width());
    descrLbl->setWordWrap(true);
    descrLbl->setText(tr("<p>The project you are about to open is located in the "
                         "write-protected location:</p><blockquote>%1</blockquote>"
                         "<p>Please select a writable location below and click \"Copy Project and Open\" "
                         "to open a modifiable copy of the project or click \"Keep Project and Open\" "
                         "to open the project in location.</p><p><b>Note:</b> You will not "
                         "be able to alter or compile your project in the current location.</p>")
                      .arg(nativeProjectDir));
    lay->addWidget(descrLbl, 0, 0, 1, 2);
    QLabel *txt = new QLabel(tr("&Location:"));
    PathChooser *chooser = new PathChooser;
    txt->setBuddy(chooser);
    chooser->setExpectedKind(PathChooser::ExistingDirectory);
    QSettings *settings = Core::ICore::settings();
    chooser->setPath(settings->value(QString::fromLatin1(C_FALLBACK_ROOT),
                                     Core::DocumentManager::projectsDirectory()).toString());
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
        QString destBaseDir = chooser->path();
        settings->setValue(QString::fromLatin1(C_FALLBACK_ROOT), destBaseDir);
        QDir toDirWithExamplesDir(destBaseDir);
        if (toDirWithExamplesDir.cd(exampleDirName)) {
            toDirWithExamplesDir.cdUp(); // step out, just to not be in the way
            QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Use Location"),
                                 tr("The specified location already exists. "
                                    "Please specify a valid location."),
                                 QMessageBox::Ok, QMessageBox::NoButton);
            return QString();
        } else {
            QString error;
            QString targetDir = destBaseDir + QLatin1Char('/') + exampleDirName;
            if (FileUtils::copyRecursively(FileName::fromString(projectDir),
                    FileName::fromString(targetDir), &error)) {
                // set vars to new location
                const QStringList::Iterator end = filesToOpen.end();
                for (QStringList::Iterator it = filesToOpen.begin(); it != end; ++it)
                    it->replace(projectDir, targetDir);

                foreach (const QString &dependency, dependencies) {
                    FileName targetFile = FileName::fromString(targetDir);
                    targetFile.appendPath(QDir(dependency).dirName());
                    if (!FileUtils::copyRecursively(FileName::fromString(dependency), targetFile,
                            &error)) {
                        QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Copy Project"), error);
                        // do not fail, just warn;
                    }
                }


                return targetDir + QLatin1Char('/') + proFileInfo.fileName();
            } else {
                QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Copy Project"), error);
            }

        }
    }
    return QString();

}

void ExamplesWelcomePage::openProject(const QString &projectFile, const QStringList &additionalFilesToOpen,
                                            const QUrl &help, const QStringList &dependencies, const QStringList &platforms)
{
    QString proFile = projectFile;
    if (proFile.isEmpty())
        return;

    QStringList filesToOpen = additionalFilesToOpen;
    QFileInfo proFileInfo(proFile);
    if (!proFileInfo.exists())
        return;

    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    if (!proFileInfo.isWritable())
        proFile = copyToAlternativeLocation(proFileInfo, filesToOpen, dependencies);

    // don't try to load help and files if loading the help request is being cancelled
    QString errorMessage;
    ProjectExplorer::ProjectExplorerPlugin *peplugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    if (proFile.isEmpty())
        return;
    if (ProjectExplorer::Project *project = peplugin->openProject(proFile, &errorMessage)) {
        Core::ICore::openFiles(filesToOpen);
        if (project->needsConfiguration())
            project->configureAsExampleProject(platforms);
        Core::ModeManager::activateModeType(Core::Constants::MODE_EDIT_TYPE);
        Core::ICore::helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
    }
    if (!errorMessage.isEmpty())
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Failed to Open Project"), errorMessage);
}

void ExamplesWelcomePage::updateTagsModel()
{
    m_engine->rootContext()->setContextProperty(QLatin1String("tagsList"), examplesModel()->tags());
    emit tagsUpdated();
}

ExamplesListModel *ExamplesWelcomePage::examplesModel() const
{
    if (examplesModelStatic())
        return examplesModelStatic().data();

    examplesModelStatic() = new ExamplesListModel(const_cast<ExamplesWelcomePage*>(this));
    return examplesModelStatic().data();
}

} // namespace Internal
} // namespace QtSupport

#include "gettingstartedwelcomepage.moc"
