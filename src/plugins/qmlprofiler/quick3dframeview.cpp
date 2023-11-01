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

#include "quick3dframeview.h"

#include "qmlprofilertr.h"

#include <coreplugin/minisplitter.h>

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QStringListModel>
#include <QLabel>

namespace QmlProfiler {
namespace Internal {

Quick3DFrameView::Quick3DFrameView(QmlProfilerModelManager *profilerModelManager, QWidget *parent)
    : QmlProfilerEventsView(parent)
{
    setObjectName(QLatin1String("QmlProfiler.Quick3DFrame.Dock"));
    setWindowTitle(Tr::tr("Quick3D Frame"));

    auto model = new Quick3DFrameModel(profilerModelManager);
    m_mainView.reset(new Quick3DMainView(model, false, this));
    connect(m_mainView.get(), &Quick3DMainView::gotoSourceLocation,
            this, &Quick3DFrameView::gotoSourceLocation);

    m_compareFrameView.reset(new Quick3DMainView(model, true, this));
    connect(m_compareFrameView.get(), &Quick3DMainView::gotoSourceLocation,
            this, &Quick3DFrameView::gotoSourceLocation);

    auto groupLayout = new QVBoxLayout(this);
    groupLayout->setContentsMargins(0,0,0,0);
    groupLayout->setSpacing(0);
    auto hMainLayout = new QHBoxLayout(this);
    hMainLayout->setContentsMargins(0,0,0,0);
    hMainLayout->setSpacing(0);
    auto hFrameLayout = new QHBoxLayout(this);
    hFrameLayout->setContentsMargins(0,0,0,0);
    hFrameLayout->setSpacing(0);
    auto view3DComboBox = new QComboBox(this);
    auto view3DComboModel = new QStringListModel(this);
    auto frameComboBox = new QComboBox(this);
    auto frameComboModel = new QStringListModel(this);
    auto selectView3DLabel = new QLabel(Tr::tr("Select View3D"), this);
    auto selectFrameLabel = new QLabel(Tr::tr("Compare Frame"), this);
    view3DComboBox->setModel(view3DComboModel);
    frameComboBox->setModel(frameComboModel);
    hFrameLayout->addWidget(selectView3DLabel);
    hFrameLayout->addWidget(view3DComboBox);
    hFrameLayout->addWidget(selectFrameLabel);
    hFrameLayout->addWidget(frameComboBox);
    groupLayout->addLayout(hFrameLayout);
    hMainLayout->addWidget(m_mainView.get());
    hMainLayout->addWidget(m_compareFrameView.get());
    groupLayout->addLayout(hMainLayout);
    connect(model, &Quick3DFrameModel::modelReset, [model, view3DComboModel, frameComboModel](){
        QStringList list;
        list << Tr::tr("All");
        list << model->view3DNames();
        view3DComboModel->setStringList(list);
        list.clear();
        list << Tr::tr("None");
        list << model->frameNames(Tr::tr("All"));
        frameComboModel->setStringList(list);
    });
    connect(view3DComboBox, &QComboBox::currentTextChanged, [this, model, frameComboModel](const QString &text){
        m_mainView->setFilterView3D(text);
        model->setFilterView3D(text);
        QStringList list;
        list << Tr::tr("None");
        list << model->frameNames(text);
        frameComboModel->setStringList(list);
    });
    connect(frameComboBox, &QComboBox::currentTextChanged, [model, this](const QString &text){
        model->setFilterFrame(text);
        m_compareFrameView->setFilterFrame(text);
    });

    setLayout(groupLayout);
}

void Quick3DFrameView::selectByTypeId(int)
{

}

void Quick3DFrameView::onVisibleFeaturesChanged(quint64)
{

}

Quick3DMainView::Quick3DMainView(Quick3DFrameModel *model, bool compareView, QWidget *parent)
    : Utils::TreeView(parent), m_model(model), m_compareView(compareView)
{
    setUniformRowHeights(false);
    setObjectName("Quick3DMainView");
    setFrameStyle(QFrame::NoFrame);
    QHeaderView *h = header();
    h->setSectionResizeMode(QHeaderView::Interactive);
    h->setDefaultSectionSize(100);
    h->setMinimumSectionSize(50);

    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model);
    sortModel->setSortRole(Quick3DFrameModel::SortRole);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortModel->setFilterRole(compareView ? Quick3DFrameModel::CompareRole : Quick3DFrameModel::FilterRole);
    if (m_compareView)
        sortModel->setFilterFixedString("+");
    setModel(sortModel);

    connect(this, &QAbstractItemView::activated, this, [this](const QModelIndex &index) {
        QString location = m_model->location(index.data(Quick3DFrameModel::IndexRole).toInt());
        if (!location.isEmpty()) {
            QString file, line;
            int lineIdx = location.lastIndexOf(QStringLiteral(".qml:"));
            int nameIdx = location.lastIndexOf(QStringLiteral(" "));
            if (lineIdx < 0)
                return;
            lineIdx += 4;
            file = location.mid(nameIdx + 1, lineIdx - nameIdx - 1);
            line = location.right(location.length() - lineIdx - 1);
            QUrl url(file);
            emit gotoSourceLocation(url.fileName(), line.toInt(), 0);
        }
    });
    m_sortModel = sortModel;

    setSortingEnabled(true);
    sortByColumn(Quick3DFrameModel::FrameGroup, Qt::AscendingOrder);

    setRootIsDecorated(true);
    setColumnWidth(0, 300);
}

void Quick3DMainView::setFilterView3D(const QString &objectName)
{
    if (objectName == Tr::tr("All"))
        m_sortModel->setFilterFixedString("");
    else
        m_sortModel->setFilterFixedString(objectName);
}

void Quick3DMainView::setFilterFrame(const QString &)
{
    m_sortModel->setFilterFixedString("+");
}

} // namespace Internal
} // namespace QmlProfiler
