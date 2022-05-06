/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include <coreplugin/icontext.h>

#include <utils/filepath.h>

#include <QtQuickWidgets/QQuickWidget>

namespace QmlProjectManager {
namespace Internal {

class DesignModeContext : public Core::IContext
{
    Q_OBJECT

public:
    DesignModeContext(QWidget *widget) { setWidget(widget); }
};

class QdsLandingPage : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool qdsInstalled MEMBER m_qdsInstalled READ qdsInstalled WRITE setQdsInstalled)
    Q_PROPERTY(bool projectFileExists MEMBER m_projectFileExists READ projectFileExists WRITE setProjectFileExists)
    Q_PROPERTY(QString qtVersion MEMBER m_qtVersion READ qtVersion WRITE setQtVersion)
    Q_PROPERTY(QString qdsVersion MEMBER m_qdsVersion READ qdsVersion WRITE setQdsVersion)

public:
    QdsLandingPage(QWidget *parent = nullptr);

    QWidget *dialog();
    void show();
    void hide();

    bool qdsInstalled() const;
    void setQdsInstalled(bool installed);
    bool projectFileExists() const;
    void setProjectFileExists(bool exists);
    const QString qtVersion() const;
    void setQtVersion(const QString &version);
    const QString qdsVersion() const;
    void setQdsVersion(const QString &version);
    const QStringList cmakeResources() const;
    void setCmakeResources(const Utils::FilePaths &resources);
    void setCmakeResources(const QStringList &resources);

signals:
    void doNotShowChanged(bool doNotShow);
    void openCreator(bool rememberSelection);
    void openDesigner(bool rememberSelection);
    void installDesigner();
    void generateCmake();
    void generateProjectFile();

private:
    QQuickWidget *m_dialog = nullptr;

    bool m_qdsInstalled = false;
    bool m_projectFileExists = false;
    Qt::CheckState m_doNotShow = Qt::Unchecked;
    QString m_qtVersion;
    QString m_qdsVersion;
    QStringList m_cmakeResources;
};

} // namespace Internal
} // namespace QmlProjectManager
