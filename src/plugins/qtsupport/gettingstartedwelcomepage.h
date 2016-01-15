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

#ifndef GETTINGSTARTEDWELCOMEPAGE_H
#define GETTINGSTARTEDWELCOMEPAGE_H

#include <coreplugin/iwelcomepage.h>

#include <QStringList>

QT_BEGIN_NAMESPACE
class QQmlEngine;
class QFileInfo;
QT_END_NAMESPACE

namespace QtSupport {
namespace Internal {

class ExamplesListModel;

class ExamplesWelcomePage : public Core::IWelcomePage
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
    Core::Id id() const;
    Q_INVOKABLE void openUrl(const QUrl &url);

public slots:
    void openHelpInExtraWindow(const QUrl &help);
    void openHelp(const QUrl &help);
    void openProject(const QString& projectFile, const QStringList& additionalFilesToOpen,
                     const QString &mainFile, const QUrl& help, const QStringList &dependencies,
                     const QStringList &platforms);

private:
    ExamplesListModel *examplesModel() const;
    QString copyToAlternativeLocation(const QFileInfo &fileInfo, QStringList &filesToOpen, const QStringList &dependencies);
    QQmlEngine *m_engine;
    bool m_showExamples;
};

} // namespace Internal
} // namespace QtSupport

#endif // GETTINGSTARTEDWELCOMEPAGE_H
