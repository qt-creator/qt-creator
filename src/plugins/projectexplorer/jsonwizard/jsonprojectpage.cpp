/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "jsonprojectpage.h"
#include "jsonwizard.h"

#include <coreplugin/documentmanager.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QVariant>

namespace ProjectExplorer {

JsonProjectPage::JsonProjectPage(QWidget *parent) : Utils::ProjectIntroPage(parent)
{ }

void JsonProjectPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);
    setPath(wiz->stringValue(QLatin1String("InitialPath")));

    setProjectName(uniqueProjectName(path()));
}

bool JsonProjectPage::validatePage()
{
    if (isComplete() && useAsDefaultPath()) {
        // Store the path as default path for new projects if desired.
        Core::DocumentManager::setProjectsDirectory(Utils::FileName::fromString(path()));
        Core::DocumentManager::setUseProjectsDirectory(true);
    }

    QString target = path();
    if (!target.endsWith(QLatin1Char('/')))
        target += QLatin1Char('/');
    target += projectName();

    wizard()->setProperty("ProjectDirectory", target);
    wizard()->setProperty("TargetPath", target);

    return Utils::ProjectIntroPage::validatePage();
}

QString JsonProjectPage::uniqueProjectName(const QString &path)
{
    const QDir pathDir(path);
    //: File path suggestion for a new project. If you choose
    //: to translate it, make sure it is a valid path name without blanks
    //: and using only ascii chars.
    const QString prefix = tr("untitled");
    for (unsigned i = 0; ; ++i) {
        QString name = prefix;
        if (i)
            name += QString::number(i);
        if (!pathDir.exists(name))
            return name;
    }
}

} // namespace ProjectExplorer
