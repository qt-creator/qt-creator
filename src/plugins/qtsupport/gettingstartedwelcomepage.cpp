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

#include <coreplugin/coreplugin.h>
#include <coreplugin/helpmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QScrollBar>
#include <QtGui/QSortFilterProxyModel>
#include <QtSql/QSqlQueryModel>
#include <QtSql/QSqlQuery>
#include <QtDeclarative>
#include <QDebug>

namespace QtSupport {
namespace Internal {

class HelpImageProvider : public QDeclarativeImageProvider
{
public:
    HelpImageProvider()
        : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
    {
    }

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
        QUrl url = QUrl::fromEncoded(id.toAscii());
        QByteArray imgData = Core::HelpManager::instance()->fileData(url);
        QBuffer imgBuffer(&imgData);
        imgBuffer.open(QIODevice::ReadOnly);
        QImageReader reader(&imgBuffer);
        QImage img = reader.read();
        if (size && requestedSize != *size)
            img = img.scaled(requestedSize);
        return img;
    }
};

GettingStartedWelcomePage::GettingStartedWelcomePage()
    : m_examplesModel(0), m_engine(0)
{
}

void GettingStartedWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    m_engine = engine;
    m_engine->addImageProvider("helpimage", new HelpImageProvider);
    m_examplesModel = new ExamplesListModel(this);
    connect (m_examplesModel, SIGNAL(tagsUpdated()), SLOT(updateTagsModel()));
    ExamplesListModelFilter *proxy = new ExamplesListModelFilter(this);
    proxy->setSourceModel(m_examplesModel);
    proxy->setDynamicSortFilter(true);
    proxy->sort(0);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QDeclarativeContext *rootContenxt = m_engine->rootContext();
    rootContenxt->setContextProperty("examplesModel", proxy);
    rootContenxt->setContextProperty("gettingStarted", this);
}

void GettingStartedWelcomePage::openSplitHelp(const QUrl &help)
{
    Core::ICore::instance()->helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
}

QStringList GettingStartedWelcomePage::tagList() const
{
    return m_examplesModel->tags();
}

void GettingStartedWelcomePage::openProject(const QString &projectFile, const QStringList &additionalFilesToOpen, const QUrl &help)
{
    qDebug() << projectFile << additionalFilesToOpen << help;
    // don't try to load help and files if loading the help request is being cancelled
    if (ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(projectFile)) {
        Core::ICore::instance()->openFiles(additionalFilesToOpen);
        Core::ICore::instance()->helpManager()->handleHelpRequest(help.toString()+QLatin1String("?view=split"));
    }
}

void GettingStartedWelcomePage::updateTagsModel()
{
    m_engine->rootContext()->setContextProperty("tagsList", m_examplesModel->tags());
    emit tagsUpdated();
}

} // namespace Internal
} // namespace QtSupport

