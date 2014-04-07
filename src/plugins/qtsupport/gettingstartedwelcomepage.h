/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef GETTINGSTARTEDWELCOMEPAGE_H
#define GETTINGSTARTEDWELCOMEPAGE_H

#include <utils/iwelcomepage.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QFileInfo;
QT_END_NAMESPACE

namespace QtSupport {
namespace Internal {

class ExamplesListModel;

class ExamplesWelcomePage : public Utils::IWelcomePage
{
    Q_OBJECT

public:
    ExamplesWelcomePage();

    void setShowExamples(bool showExamples);
    QUrl pageLocation() const;
    QString title() const;
    int priority() const;
    bool hasSearchBar() const;
    void facilitateQml(QQmlEngine *);
    Id id() const;
    Q_INVOKABLE void openUrl(const QUrl &url);

public slots:
    void openSplitHelp(const QUrl &help);
    void openHelp(const QUrl &help);
    void openProject(const QString& projectFile, const QStringList& additionalFilesToOpen,
                     const QUrl& help, const QStringList &dependencies, const QStringList &platforms);

private:
    ExamplesListModel *examplesModel() const;
    QString copyToAlternativeLocation(const QFileInfo &fileInfo, QStringList &filesToOpen, const QStringList &dependencies);
    QQmlEngine *m_engine;
    bool m_showExamples;
};

} // namespace Internal
} // namespace QtSupport

#endif // GETTINGSTARTEDWELCOMEPAGE_H
