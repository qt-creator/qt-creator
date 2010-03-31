/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "openpagesmanager.h"

#include "centralwidget.h"
#include "helpconstants.h"
#include "helpmanager.h"
#include "helpviewer.h"
#include "openpagesmodel.h"
#include "openpageswidget.h"

#include <QtGui/QComboBox>
#include <QtGui/QTreeView>

#include <QtHelp/QHelpEngineCore>

using namespace Help::Internal;

OpenPagesManager *OpenPagesManager::m_instance = 0;

// -- OpenPagesManager

OpenPagesManager::OpenPagesManager(QObject *parent)
    : QObject(parent)
    , m_comboBox(0)
    , m_model(0)
    , m_openPagesWidget(0)
{
    Q_ASSERT(!m_instance);

    m_instance = this;
    m_model = new OpenPagesModel(this);

    m_openPagesWidget = new OpenPagesWidget(m_model);
    connect(m_openPagesWidget, SIGNAL(setCurrentPage(QModelIndex)), this,
        SLOT(setCurrentPage(QModelIndex)));
    connect(m_openPagesWidget, SIGNAL(closePage(QModelIndex)), this,
        SLOT(closePage(QModelIndex)));
    connect(m_openPagesWidget, SIGNAL(closePagesExcept(QModelIndex)), this,
        SLOT(closePagesExcept(QModelIndex)));

    m_comboBox = new QComboBox;
    m_comboBox->setModel(m_model);
    m_comboBox->setMinimumContentsLength(40);
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(setCurrentPage(int)));
}

OpenPagesManager &OpenPagesManager::instance()
{
    Q_ASSERT(m_instance);
    return *m_instance;
}

QWidget* OpenPagesManager::openPagesWidget() const
{
    return m_openPagesWidget;
}

QComboBox* OpenPagesManager::openPagesComboBox() const
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
    const QHelpEngineCore &engine = HelpManager::helpEngineCore();
    const int option = engine.customValue(QLatin1String("StartOption"),
        Help::Constants::ShowLastPages).toInt();
    QString homePage = engine.customValue(QLatin1String("DefaultHomePage"),
        Help::Constants::AboutBlank).toString();

    int initialPage = 0;
    switch (option) {
        case Help::Constants::ShowHomePage: {
            m_model->addPage(engine.customValue(QLatin1String("HomePage"),
                homePage).toString());
        }   break;

        case Help::Constants::ShowBlankPage: {
            m_model->addPage(QUrl(Help::Constants::AboutBlank));
        }   break;

        case Help::Constants::ShowLastPages: {
            const QStringList &lastShownPageList = splitString(engine
                .customValue(QLatin1String("LastShownPages")));
            const int pageCount = lastShownPageList.count();

            if (pageCount > 0) {
                QStringList zoomFactors = splitString(engine
                    .customValue(QLatin1String("LastShownPagesZoom")));
                while (zoomFactors.count() < pageCount)
                    zoomFactors.append(Help::Constants::DefaultZoomFactor);

                initialPage = engine.customValue(QLatin1String("LastTabPage"), 0).toInt();
                for (int curPage = 0; curPage < pageCount; ++curPage) {
                    const QString &curFile = lastShownPageList.at(curPage);
                    if (engine.findFile(curFile).isValid()
                        || curFile == Help::Constants::AboutBlank) {
                        m_model->addPage(curFile, zoomFactors.at(curPage).toFloat());
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
        CentralWidget::instance()->addPage(m_model->pageAt(i));

    emit pagesChanged();
    setCurrentPage(initialPage);
}

// -- public slots

HelpViewer *OpenPagesManager::createPage()
{
    return createPage(QUrl(Help::Constants::AboutBlank));
}

HelpViewer *OpenPagesManager::createPageFromSearch(const QUrl &url)
{
    return createPage(url, true);
}

HelpViewer *OpenPagesManager::createPage(const QUrl &url, bool fromSearch)
{
    if (HelpViewer::launchWithExternalApp(url))
        return 0;

    m_model->addPage(url);

    const int index = m_model->rowCount() - 1;
    HelpViewer * const page = m_model->pageAt(index);
    CentralWidget::instance()->addPage(page, fromSearch);

    emit pagesChanged();
    setCurrentPage(index);

    return page;
}

void OpenPagesManager::setCurrentPage(int index)
{
    m_comboBox->setCurrentIndex(index);
    CentralWidget::instance()->setCurrentPage(m_model->pageAt(index));

    selectCurrentPage();    // update the selection inside the tree view
}

void OpenPagesManager::setCurrentPage(const QModelIndex &index)
{
    if (index.isValid()) {
        m_comboBox->setCurrentIndex(index.row());
        CentralWidget::instance()->setCurrentPage(m_model->pageAt(index.row()));
    }
}

void OpenPagesManager::closeCurrentPage()
{
    QModelIndexList indexes = m_openPagesWidget->selectionModel()->selectedRows();
    if (indexes.isEmpty())
        return;
    Q_ASSERT(indexes.count() == 1);
    removePage(indexes.first().row());
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
            if (m_model->pageAt(i) != viewer) {
                removePage(i);
            } else {
                i++;
            }
        }
    }
}

// -- private

void OpenPagesManager::selectCurrentPage()
{
    QItemSelectionModel * selModel = m_openPagesWidget->selectionModel();
    selModel->clearSelection();
    selModel->select(m_model->index(CentralWidget::instance()->currentIndex(), 0),
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_openPagesWidget->scrollTo(m_openPagesWidget->currentIndex());
}

void OpenPagesManager::removePage(int index)
{
    Q_ASSERT(m_model->rowCount() > 1);

    m_model->removePage(index);
    CentralWidget::instance()->removePage(index);

    emit pagesChanged();
    selectCurrentPage();    // update the selection inside the tree view
}
