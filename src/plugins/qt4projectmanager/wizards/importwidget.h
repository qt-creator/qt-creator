/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef IMPORTWIDGET_H
#define IMPORTWIDGET_H

#include <QWidget>

#include <utils/fileutils.h>

namespace Utils { class PathChooser; }

namespace Qt4ProjectManager {
namespace Internal {

class ImportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImportWidget(QWidget *parent = 0);
    ~ImportWidget();

    void setCurrentDirectory(const Utils::FileName &dir);

signals:
    void importFrom(const Utils::FileName &dir);

private slots:
    void handleImportRequest();

private:
    Utils::PathChooser *m_pathChooser;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // IMPORTWIDGET_H
