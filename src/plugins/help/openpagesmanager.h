// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QListView;
class QModelIndex;
class QPoint;
class QUrl;
class QWidget;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class HelpWidget;
class HelpViewer;
class OpenPagesSwitcher;
class OpenPagesWidget;

class OpenPagesManager : public QObject
{
    Q_OBJECT

public:
    OpenPagesManager(HelpWidget *helpWidget);
    ~OpenPagesManager() override;

    QWidget *openPagesWidget() const;
    QComboBox *openPagesComboBox() const;

    void setupInitialPages();

    void closeCurrentPage();
    void closePage(const QModelIndex &index);
    void closePagesExcept(const QModelIndex &index);

    void gotoNextPage();
    void gotoPreviousPage();

private:
    void removePage(int index);
    void showTwicherOrSelectPage() const;
    void openPagesContextMenu(const QPoint &point);

    QComboBox *m_comboBox = nullptr;
    HelpWidget *m_helpWidget = nullptr;
    mutable OpenPagesWidget *m_openPagesWidget = nullptr;
    OpenPagesSwitcher *m_openPagesSwitcher = nullptr;
};

} // namespace Internal
} // namespace Help
