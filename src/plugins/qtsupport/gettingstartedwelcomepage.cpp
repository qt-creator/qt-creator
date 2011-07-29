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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "gettingstartedwelcomepage.h"

#include "exampleslistmodel.h"

#include <utils/pathchooser.h>
#include <utils/fileutils.h>

#include <coreplugin/coreplugin.h>
#include <coreplugin/helpmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QScrollBar>
#include <QtGui/QSortFilterProxyModel>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlQuery>
#include <QtDeclarative>
#include <QDebug>

namespace QtSupport {
namespace Internal {

const char *C_FALLBACK_ROOT = "ProjectsFallbackRoot";

class Fetcher : public QObject
{
    Q_OBJECT
public slots:
    void fetchData(const QUrl &url)
    {
        fetchedData = Core::HelpManager::instance()->fileData(url);
    }

public:
    QByteArray fetchedData;
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
        QMutexLocker lock(&m_mutex);
        QUrl url = QUrl::fromEncoded(id.toAscii());
        if (!QMetaObject::invokeMethod(&m_fetcher,
                                       "fetchData",
                                       Qt::BlockingQueuedConnection,
                                       Q_ARG(QUrl, url))) {
            return QImage();
        }
        QBuffer imgBuffer(&m_fetcher.fetchedData);
        imgBuffer.open(QIODevice::ReadOnly);
        QImageReader reader(&imgBuffer);
        QImage img = reader.read();
        if (size && requestedSize != *size)
            img = img.scaled(requestedSize);
        m_fetcher.fetchedData.clear();
        return img;
    }
private:
    Fetcher m_fetcher;
    QMutex m_mutex;
};

GettingStartedWelcomePage::GettingStartedWelcomePage()
    : m_examplesModel(0), m_engine(0)
{
}

void GettingStartedWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    m_engine = engine;
    m_engine->addImageProvider(QLatin1String("helpimage"), new HelpImageProvider);
    m_examplesModel = new ExamplesListModel(this);
    connect (m_examplesModel, SIGNAL(tagsUpdated()), SLOT(updateTagsModel()));
    ExamplesListModelFilter *proxy = new ExamplesListModelFilter(this);
    proxy->setSourceModel(m_examplesModel);
    proxy->setDynamicSortFilter(true);
    proxy->sort(0);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QDeclarativeContext *rootContenxt = m_engine->rootContext();
    rootContenxt->setContextProperty(QLatin1String("examplesModel"), proxy);
    rootContenxt->setContextProperty(QLatin1String("gettingStarted"), this);
}

void GettingStartedWelcomePage::openSplitHelp(const QUrl &help)
{
    Core::ICore::instance()->helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
}

QStringList GettingStartedWelcomePage::tagList() const
{
    return m_examplesModel->tags();
}

QString GettingStartedWelcomePage::copyToAlternativeLocation(const QFileInfo& proFileInfo, QStringList &filesToOpen)
{
    const QString projectDir = proFileInfo.canonicalPath();
    QDialog d(Core::ICore::instance()->mainWindow());
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
                      .arg(QDir::toNativeSeparators(projectDir)));
    lay->addWidget(descrLbl, 0, 0, 1, 2);
    QLabel *txt = new QLabel(tr("&Location:"));
    Utils::PathChooser *chooser = new Utils::PathChooser;
    txt->setBuddy(chooser);
    chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    QSettings *settings = Core::ICore::instance()->settings();
    chooser->setPath(settings->value(
                         QString::fromLatin1(C_FALLBACK_ROOT), QDir::homePath()).toString());
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
            QMessageBox::warning(Core::ICore::instance()->mainWindow(), tr("Cannot Use Location"),
                                 tr("The specified location already exists. "
                                    "Please specify a valid location."),
                                 QMessageBox::Ok, QMessageBox::NoButton);
            return QString();
        } else {
            QString error;
            QString targetDir = destBaseDir + '/' + exampleDirName;
            if (Utils::FileUtils::copyRecursively(projectDir, targetDir, &error)) {
                // set vars to new location
                QStringList::Iterator it;
                for (it = filesToOpen.begin(); it != filesToOpen.end(); ++it)
                    it->replace(projectDir, targetDir);

                return targetDir+ '/' + proFileInfo.fileName();
            } else {
                QMessageBox::warning(Core::ICore::instance()->mainWindow(), tr("Cannot Copy Project"), error);
            }

        }
    }
    return QString();

}

void GettingStartedWelcomePage::openProject(const QString &projectFile, const QStringList &additionalFilesToOpen, const QUrl &help)
{
    QString proFile = projectFile;
    if (proFile.isEmpty())
        return;

    QStringList filesToOpen = additionalFilesToOpen;
    QFileInfo proFileInfo(proFile);
    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    if (!proFileInfo.isWritable())
        proFile = copyToAlternativeLocation(proFileInfo, filesToOpen);

    // don't try to load help and files if loading the help request is being cancelled
    if (!proFile.isEmpty() && ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFile)) {
        Core::ICore::instance()->openFiles(filesToOpen);
        Core::ICore::instance()->helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
    }
}

void GettingStartedWelcomePage::updateTagsModel()
{
    m_engine->rootContext()->setContextProperty(QLatin1String("tagsList"), m_examplesModel->tags());
    emit tagsUpdated();
}

} // namespace Internal
} // namespace QtSupport

#include "gettingstartedwelcomepage.moc"
