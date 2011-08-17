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

#ifndef GETTINGSTARTEDWELCOMEPLUGIN_H
#define GETTINGSTARTEDWELCOMEPLUGIN_H

#include <utils/iwelcomepage.h>

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
class QFileInfo;
QT_END_NAMESPACE

namespace QtSupport {
namespace Internal {

class ExamplesListModel;
class GettingStartedWelcomePageWidget;


class GettingStartedWelcomePage : public Utils::IWelcomePage
{
    Q_OBJECT
public:
    GettingStartedWelcomePage();

    QUrl pageLocation() const;
    QString title() const { return tr("Getting Started");}
    int priority() const { return 10; }
    void facilitateQml(QDeclarativeEngine *);
    Q_INVOKABLE QStringList tagList() const;

signals:
    void tagsUpdated();

public slots:
    void openSplitHelp(const QUrl &help);
    void openProject(const QString& projectFile, const QStringList& additionalFilesToOpen, const QUrl& help);
    void updateTagsModel();

private:
    QString copyToAlternativeLocation(const QFileInfo &fileInfo, QStringList &filesToOpen);
    ExamplesListModel *m_examplesModel;
    QDeclarativeEngine *m_engine;
};

} // namespace Internal
} // namespace QtSupport

#endif // GETTINGSTARTEDWELCOMEPLUGIN_H
