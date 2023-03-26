// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openpagesmanager.h"

#include "helpconstants.h"
#include "helptr.h"
#include "helpviewer.h"
#include "helpwidget.h"
#include "localhelpmanager.h"
#include "openpagesswitcher.h"
#include "openpageswidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QMenu>

#include <QHelpEngine>

using namespace Core;

namespace Help::Internal {

// -- OpenPagesManager

OpenPagesManager::OpenPagesManager(HelpWidget *helpWidget)
    : m_helpWidget(helpWidget)
{
    m_comboBox = new QComboBox;
    m_comboBox->setModel(m_helpWidget->model());
    m_comboBox->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_comboBox, &QComboBox::activated, m_helpWidget, &HelpWidget::setCurrentIndex);
    connect(m_helpWidget, &HelpWidget::currentIndexChanged, m_comboBox, &QComboBox::setCurrentIndex);
    connect(m_comboBox, &QWidget::customContextMenuRequested,
            this, &OpenPagesManager::openPagesContextMenu);

    m_openPagesSwitcher = new OpenPagesSwitcher(m_helpWidget->model());
    connect(m_openPagesSwitcher, &OpenPagesSwitcher::closePage, this,
        &OpenPagesManager::closePage);
    connect(m_openPagesSwitcher,
            &OpenPagesSwitcher::setCurrentPage,
            m_helpWidget,
            [this](const QModelIndex &index) { m_helpWidget->setCurrentIndex(index.row()); });
    connect(m_helpWidget,
            &HelpWidget::currentIndexChanged,
            m_openPagesSwitcher,
            &OpenPagesSwitcher::selectCurrentPage);
}

OpenPagesManager ::~OpenPagesManager()
{
    delete m_openPagesSwitcher;
}

QWidget *OpenPagesManager::openPagesWidget() const
{
    if (!m_openPagesWidget) {
        m_openPagesWidget = new OpenPagesWidget(m_helpWidget->model());
        connect(m_openPagesWidget,
                &OpenPagesWidget::setCurrentPage,
                m_helpWidget,
                [this](const QModelIndex &index) { m_helpWidget->setCurrentIndex(index.row()); });
        connect(m_helpWidget,
                &HelpWidget::currentIndexChanged,
                m_openPagesWidget,
                &OpenPagesWidget::selectCurrentPage);
        connect(m_openPagesWidget, &OpenPagesWidget::closePage,
                this, &OpenPagesManager::closePage);
        connect(m_openPagesWidget, &OpenPagesWidget::closePagesExcept,
                this, &OpenPagesManager::closePagesExcept);
    }
    return m_openPagesWidget;
}

QComboBox *OpenPagesManager::openPagesComboBox() const
{
    return m_comboBox;
}

QStringList splitString(const QVariant &value)
{
    using namespace Help::Constants;
    return value.toString().split(ListSeparator, Qt::SkipEmptyParts);
}

void OpenPagesManager::setupInitialPages()
{
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    const LocalHelpManager::StartOption option = LocalHelpManager::startOption();
    QString homePage = LocalHelpManager::homePage();

    int initialPage = 0;
    switch (option) {
    case LocalHelpManager::ShowHomePage:
        m_helpWidget->addViewer(homePage);
        break;

    case LocalHelpManager::ShowBlankPage:
        m_helpWidget->addViewer(QUrl(Help::Constants::AboutBlank));
        break;

    case LocalHelpManager::ShowLastPages: {
        const QStringList &lastShownPageList = LocalHelpManager::lastShownPages();
        const int pageCount = lastShownPageList.count();

        if (pageCount > 0) {
            initialPage = LocalHelpManager::lastSelectedTab();
            for (int curPage = 0; curPage < pageCount; ++curPage) {
                const QString &curFile = lastShownPageList.at(curPage);
                if (engine.findFile(curFile).isValid() || curFile == Help::Constants::AboutBlank) {
                    m_helpWidget->addViewer(curFile);
                } else if (curPage <= initialPage && initialPage > 0) {
                    --initialPage;
                }
            }
        }
    } break;

    default:
        break;
    }

    if (m_helpWidget->viewerCount() == 0)
        m_helpWidget->addViewer(homePage);

    m_helpWidget->setCurrentIndex(std::max(initialPage, m_helpWidget->viewerCount() - 1));
}

void OpenPagesManager::closeCurrentPage()
{
    if (!m_openPagesWidget)
        return;

    QModelIndexList indexes = m_openPagesWidget->selectionModel()->selectedRows();
    if (indexes.isEmpty())
        return;

    const bool returnOnClose = LocalHelpManager::returnOnClose();

    if (m_helpWidget->viewerCount() == 1 && returnOnClose) {
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
    } else {
        QTC_ASSERT(indexes.count() == 1, return );
        removePage(indexes.first().row());
    }
}

void OpenPagesManager::closePage(const QModelIndex &index)
{
    if (index.isValid())
        removePage(index.row());
}

void OpenPagesManager::closePagesExcept(const QModelIndex &index)
{
    if (index.isValid()) {
        int i = 0;
        HelpViewer *viewer = m_helpWidget->viewerAt(index.row());
        while (m_helpWidget->viewerCount() > 1) {
            if (m_helpWidget->viewerAt(i) != viewer)
                removePage(i);
            else
                i++;
        }
    }
}

void OpenPagesManager::gotoNextPage()
{
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->gotoNextPage();
        showTwicherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoNextPage();
    }
}

void OpenPagesManager::gotoPreviousPage()
{
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->gotoPreviousPage();
        showTwicherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoPreviousPage();
    }
}

// -- private

void OpenPagesManager::removePage(int index)
{
    QTC_ASSERT(index < m_helpWidget->viewerCount(), return );

    m_helpWidget->removeViewerAt(index);
}

void OpenPagesManager::showTwicherOrSelectPage() const
{
    if (QApplication::keyboardModifiers() != Qt::NoModifier) {
        const int width = m_helpWidget->width();
        const int height = m_helpWidget->height();
        const QPoint p(m_helpWidget->mapToGlobal(QPoint(0, 0)));
        m_openPagesSwitcher->move((width - m_openPagesSwitcher->width()) / 2 + p.x(),
            (height - m_openPagesSwitcher->height()) / 2 + p.y());
        m_openPagesSwitcher->setVisible(true);
    } else {
        m_openPagesSwitcher->selectAndHide();
    }
}

void OpenPagesManager::openPagesContextMenu(const QPoint &point)
{
    const QModelIndex &index = m_helpWidget->model()->index(m_comboBox->currentIndex(), 0);
    const QString &fileName = m_helpWidget->model()->data(index, Qt::ToolTipRole).toString();
    if (fileName.isEmpty())
        return;

    QMenu menu;
    menu.addAction(Tr::tr("Copy Full Path to Clipboard"));
    if (menu.exec(m_comboBox->mapToGlobal(point)))
        Utils::setClipboardAndSelection(fileName);
}

} // Help::Internal
