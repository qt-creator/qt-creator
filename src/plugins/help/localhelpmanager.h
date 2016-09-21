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

#pragma once

#include <coreplugin/helpmanager.h>

#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QUrl>
#include <QStandardItemModel>

QT_FORWARD_DECLARE_CLASS(QHelpEngine)

class BookmarkManager;

namespace Help {
namespace Internal {

class LocalHelpManager : public QObject
{
    Q_OBJECT

public:
    struct HelpData {
        QUrl resolvedUrl;
        QByteArray data;
        QString mimeType;
    };

    enum StartOption {
        ShowHomePage = 0,
        ShowBlankPage = 1,
        ShowLastPages = 2,
    };

    LocalHelpManager(QObject *parent = 0);
    ~LocalHelpManager();

    static LocalHelpManager *instance();

    static QString defaultHomePage();
    static QString homePage();
    static void setHomePage(const QString &page);

    static QFont fallbackFont();
    static void setFallbackFont(const QFont &font);

    static StartOption startOption();
    static void setStartOption(StartOption option);

    static Core::HelpManager::HelpViewerLocation contextHelpOption();
    static void setContextHelpOption(Core::HelpManager::HelpViewerLocation location);

    static bool returnOnClose();
    static void setReturnOnClose(bool returnOnClose);

    static QStringList lastShownPages();
    static void setLastShownPages(const QStringList &pages);

    static QList<float> lastShownPagesZoom();
    static void setLastShownPagesZoom(const QList<qreal> &zoom);

    static int lastSelectedTab();
    static void setLastSelectedTab(int index);

    static void setupGuiHelpEngine();
    static void setEngineNeedsUpdate();

    static QHelpEngine& helpEngine();
    static BookmarkManager& bookmarkManager();

    static bool isValidUrl(const QString &link);

    static QByteArray loadErrorMessage(const QUrl &url, const QString &errorString);
    Q_INVOKABLE static Help::Internal::LocalHelpManager::HelpData helpData(const QUrl &url);

    static QAbstractItemModel *filterModel();
    static void setFilterIndex(int index);
    static int filterIndex();

    static void updateFilterModel();

signals:
    void filterIndexChanged(int index);
    void fallbackFontChanged(const QFont &font);
    void returnOnCloseChanged();

private:
    static bool m_guiNeedsSetup;
    static bool m_needsCollectionFile;

    static QStandardItemModel *m_filterModel;
    static QString m_currentFilter;
    static int m_currentFilterIndex;

    static QMutex m_guiMutex;
    static QHelpEngine *m_guiEngine;

    static QMutex m_bkmarkMutex;
    static BookmarkManager *m_bookmarkManager;
};

}   // Internal
}   // Help

Q_DECLARE_METATYPE(Help::Internal::LocalHelpManager::HelpData)
