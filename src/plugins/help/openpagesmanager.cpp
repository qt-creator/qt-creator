/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "openpagesmanager.h"

#include "centralwidget.h"
#include "helpconstants.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "openpagesmodel.h"
#include "openpagesswitcher.h"
#include "openpageswidget.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QMenu>

#include <QHelpEngine>

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

using namespace Core;
using namespace Help::Internal;

OpenPagesManager *OpenPagesManager::m_instance = 0;

// -- OpenPagesManager

OpenPagesManager::OpenPagesManager(QObject *parent)
    : QObject(parent)
    , m_comboBox(0)
    , m_model(0)
    , m_openPagesWidget(0)
    , m_openPagesSwitcher(0)
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    m_model = new OpenPagesModel(this);

    m_comboBox = new QComboBox;
    m_comboBox->setModel(m_model);
    m_comboBox->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_comboBox, SIGNAL(activated(int)), this, SLOT(setCurrentPage(int)));
    connect(m_comboBox, SIGNAL(customContextMenuRequested(QPoint)), this,
        SLOT(openPagesContextMenu(QPoint)));

    m_openPagesSwitcher = new OpenPagesSwitcher(m_model);
    connect(m_openPagesSwitcher, SIGNAL(closePage(QModelIndex)), this,
        SLOT(closePage(QModelIndex)));
    connect(m_openPagesSwitcher, SIGNAL(setCurrentPage(QModelIndex)), this,
        SLOT(setCurrentPage(QModelIndex)));
}

OpenPagesManager ::~OpenPagesManager()
{
    m_instance = 0;
    delete m_openPagesSwitcher;
}

OpenPagesManager &OpenPagesManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

QWidget *OpenPagesManager::openPagesWidget() const
{
    if (!m_openPagesWidget) {
        m_openPagesWidget = new OpenPagesWidget(m_model);
        connect(m_openPagesWidget, SIGNAL(setCurrentPage(QModelIndex)), this,
            SLOT(setCurrentPage(QModelIndex)));
        connect(m_openPagesWidget, SIGNAL(closePage(QModelIndex)), this,
            SLOT(closePage(QModelIndex)));
        connect(m_openPagesWidget, SIGNAL(closePagesExcept(QModelIndex)), this,
            SLOT(closePagesExcept(QModelIndex)));
    }
    return m_openPagesWidget;
}

QComboBox *OpenPagesManager::openPagesComboBox() const
{
    return m_comboBox;
}

int OpenPagesManager::pageCount() const
{
    return m_model->rowCount();
}

QStringList splitString(const QVariant &value)
{
    using namespace Help::Constants;
    return value.toString().split(ListSeparator, QString::SkipEmptyParts);
}

void OpenPagesManager::setupInitialPages()
{
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    const LocalHelpManager::StartOption option = LocalHelpManager::startOption();
    QString homePage = LocalHelpManager::homePage();

    int initialPage = 0;
    switch (option) {
        case LocalHelpManager::ShowHomePage: {
            m_model->addPage(homePage);
        }   break;

        case LocalHelpManager::ShowBlankPage: {
            m_model->addPage(QUrl(Help::Constants::AboutBlank));
        }   break;

        case LocalHelpManager::ShowLastPages: {
            const QStringList &lastShownPageList = LocalHelpManager::lastShownPages();
            const int pageCount = lastShownPageList.count();

            if (pageCount > 0) {
                QList<float> zoomFactors = LocalHelpManager::lastShownPagesZoom();
                while (zoomFactors.count() < pageCount)
                    zoomFactors.append(0.);

                initialPage = LocalHelpManager::lastSelectedTab();
                for (int curPage = 0; curPage < pageCount; ++curPage) {
                    const QString &curFile = lastShownPageList.at(curPage);
                    if (engine.findFile(curFile).isValid()
                        || curFile == Help::Constants::AboutBlank) {
                        m_model->addPage(curFile, zoomFactors.at(curPage));
                    } else if (curPage <= initialPage && initialPage > 0) {
                        --initialPage;
                    }
                }
            }
        }   break;

        default: break;
    }

    if (m_model->rowCount() == 0)
        m_model->addPage(homePage);

    for (int i = 0; i < m_model->rowCount(); ++i)
        CentralWidget::instance()->addViewer(m_model->pageAt(i));

    emit pagesChanged();
    setCurrentPage((initialPage >= m_model->rowCount())
        ? m_model->rowCount() - 1 : initialPage);
    m_openPagesSwitcher->selectCurrentPage();
}

// -- public slots

HelpViewer *OpenPagesManager::createPage()
{
    return createPage(QUrl(Help::Constants::AboutBlank));
}

HelpViewer *OpenPagesManager::createPage(const QUrl &url)
{
    if (url.isValid() && HelpViewer::launchWithExternalApp(url))
        return 0;

    m_model->addPage(url);

    const int index = m_model->rowCount() - 1;
    HelpViewer * const page = m_model->pageAt(index);
    CentralWidget::instance()->addViewer(page);

    emit pagesChanged();
    setCurrentPage(index);

    return page;
}

void OpenPagesManager::setCurrentPage(int index)
{
    CentralWidget::instance()->setCurrentViewer(m_model->pageAt(index));

    m_comboBox->setCurrentIndex(index);
    if (m_openPagesWidget)
        m_openPagesWidget->selectCurrentPage();
}

void OpenPagesManager::setCurrentPage(const QModelIndex &index)
{
    if (index.isValid())
        setCurrentPage(index.row());
}

void OpenPagesManager::closeCurrentPage()
{
    if (!m_openPagesWidget)
        return;

    QModelIndexList indexes = m_openPagesWidget->selectionModel()->selectedRows();
    if (indexes.isEmpty())
        return;

    const bool returnOnClose = LocalHelpManager::returnOnClose();

    if (m_model->rowCount() == 1 && returnOnClose) {
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
    } else {
        Q_ASSERT(indexes.count() == 1);
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
        HelpViewer *viewer = m_model->pageAt(index.row());
        while (m_model->rowCount() > 1) {
            if (m_model->pageAt(i) != viewer)
                removePage(i);
            else
                i++;
        }
    }
}

void OpenPagesManager::gotoNextPage()
{
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->selectCurrentPage();
        m_openPagesSwitcher->gotoNextPage();
        showTwicherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoNextPage();
    }
}

void OpenPagesManager::gotoPreviousPage()
{
    if (!m_openPagesSwitcher->isVisible()) {
        m_openPagesSwitcher->selectCurrentPage();
        m_openPagesSwitcher->gotoPreviousPage();
        showTwicherOrSelectPage();
    } else {
        m_openPagesSwitcher->gotoPreviousPage();
    }
}

// -- private

void OpenPagesManager::removePage(int index)
{
    Q_ASSERT(m_model->rowCount() > 1);

    m_model->removePage(index);
    CentralWidget::instance()->removeViewerAt(index);

    emit pagesChanged();
    if (m_openPagesWidget)
        m_openPagesWidget->selectCurrentPage();
}

void OpenPagesManager::showTwicherOrSelectPage() const
{
    if (QApplication::keyboardModifiers() != Qt::NoModifier) {
        const int width = CentralWidget::instance()->width();
        const int height = CentralWidget::instance()->height();
        const QPoint p(CentralWidget::instance()->mapToGlobal(QPoint(0, 0)));
        m_openPagesSwitcher->move((width - m_openPagesSwitcher->width()) / 2 + p.x(),
            (height - m_openPagesSwitcher->height()) / 2 + p.y());
        m_openPagesSwitcher->setVisible(true);
    } else {
        m_openPagesSwitcher->selectAndHide();
    }
}

// -- private slots

void OpenPagesManager::openPagesContextMenu(const QPoint &point)
{
    const QModelIndex &index = m_model->index(m_comboBox->currentIndex(), 0);
    const QString &fileName = m_model->data(index, Qt::ToolTipRole).toString();
    if (fileName.isEmpty())
        return;

    QMenu menu;
    menu.addAction(tr("Copy Full Path to Clipboard"));
    if (menu.exec(m_comboBox->mapToGlobal(point)))
        QApplication::clipboard()->setText(fileName);
}
