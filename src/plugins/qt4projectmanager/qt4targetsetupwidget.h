/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4TARGETSETUPWIDGET_H
#define QT4TARGETSETUPWIDGET_H

#include "qt4projectmanager_global.h"

#include <QtGui/QWidget>

namespace Qt4ProjectManager {
struct BuildConfigurationInfo;

class QT4PROJECTMANAGER_EXPORT Qt4TargetSetupWidget : public QWidget
{
    Q_OBJECT
public:
    explicit Qt4TargetSetupWidget(QWidget *parent = 0);
    ~Qt4TargetSetupWidget();
    virtual bool isTargetSelected() const = 0;
    virtual void setTargetSelected(bool b) = 0;
    virtual void setProFilePath(const QString &proFilePath) = 0;
    virtual QList<BuildConfigurationInfo> usedImportInfos() = 0;

signals:
    void selectedToggled() const;
    void newImportBuildConfiguration(const BuildConfigurationInfo &info);
};

} // namespace Qt4ProjectManager

#endif // QT4TARGETSETUPWIDGET_H
