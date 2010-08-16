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

#include "cpptypehierarchy.h"
#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppelementevaluator.h"
#include "cppplugin.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/navigationtreeview.h>

#include <QtCore/QLatin1Char>
#include <QtCore/QLatin1String>
#include <QtCore/QModelIndex>
#include <QtCore/QSettings>
#include <QtGui/QVBoxLayout>
#include <QtGui/QStandardItemModel>
#include <QtGui/QFontMetrics>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QLabel>

using namespace CppEditor;
using namespace Internal;

// CppTypeHierarchyItem
CppTypeHierarchyItem::CppTypeHierarchyItem(const CppClass &cppClass) :
    QStandardItem(), m_cppClass(cppClass)
{}

CppTypeHierarchyItem::~CppTypeHierarchyItem()
{}

int CppTypeHierarchyItem::type() const
{ return UserType; }

const CppClass &CppTypeHierarchyItem::cppClass() const
{ return m_cppClass; }

// CppTypeHierarchyDelegate
CppTypeHierarchyDelegate::CppTypeHierarchyDelegate(QObject *parent) : QStyledItemDelegate(parent)
{}

CppTypeHierarchyDelegate::~CppTypeHierarchyDelegate()
{}

void CppTypeHierarchyDelegate::paint(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    if (const QStyleOptionViewItemV3 *v3 =
            qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option)) {
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, v3, painter, v3->widget);

        QStyleOptionViewItemV4 opt = option;
        initStyleOption(&opt, index);

        const QStandardItemModel *model = static_cast<const QStandardItemModel *>(index.model());
        CppTypeHierarchyItem *item =
            static_cast<CppTypeHierarchyItem *>(model->itemFromIndex(index));

        painter->save();
        const QIcon &icon = item->cppClass().icon();
        const QSize &iconSize = icon.actualSize(opt.decorationSize);
        QRect workingRect(opt.rect);
        QRect decorationRect(workingRect.topLeft(), iconSize);
        icon.paint(painter, decorationRect, opt.decorationAlignment);
        workingRect.setX(workingRect.x() + iconSize.width() + 4);

        QRect boundingRect;
        const QString &name = item->cppClass().name() + QLatin1Char(' ');
        painter->drawText(workingRect, Qt::AlignLeft, name, &boundingRect);
        if (item->cppClass().name() != item->cppClass().qualifiedName()) {
            QFont font(painter->font());
            if (font.pointSize() > 2)
                font.setPointSize(font.pointSize() - 2);
            else if (font.pointSize() > 1)
                font.setPointSize(font.pointSize() - 1);
            font.setItalic(true);
            painter->setFont(font);

            QFontMetrics metrics(font);
            workingRect.setX(boundingRect.x() + boundingRect.width());
            workingRect.setY(boundingRect.y() + boundingRect.height() - metrics.height());
            painter->drawText(workingRect, Qt::AlignLeft, item->cppClass().qualifiedName());
        }
        painter->restore();
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize CppTypeHierarchyDelegate::sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.rwidth() += 5; // Extend a bit because of the font processing.
    return size;
}

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget(Core::IEditor *editor) :
    QWidget(0),
    m_cppEditor(0),
    m_treeView(0),
    m_model(0),
    m_delegate(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    if (CPPEditorEditable *cppEditable = qobject_cast<CPPEditorEditable *>(editor)) {
        m_cppEditor = static_cast<CPPEditor *>(cppEditable->widget());

        m_model = new QStandardItemModel;
        m_treeView = new Utils::NavigationTreeView;
        m_delegate = new CppTypeHierarchyDelegate;
        m_treeView->setModel(m_model);
        m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_treeView->setItemDelegate(m_delegate);
        layout->addWidget(m_treeView);

        connect(m_treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(onItemClicked(QModelIndex)));
        connect(CppPlugin::instance(), SIGNAL(typeHierarchyRequested()), this, SLOT(perform()));
    } else {
        QLabel *label = new QLabel(tr("No type hierarchy available"), this);
        label->setAlignment(Qt::AlignCenter);
        label->setAutoFillBackground(true);
        label->setBackgroundRole(QPalette::Base);
        layout->addWidget(label);
    }
    setLayout(layout);
}

CppTypeHierarchyWidget::~CppTypeHierarchyWidget()
{
    delete m_model;
    delete m_delegate;
}

bool CppTypeHierarchyWidget::handleEditorChange(Core::IEditor *editor)
{
    if (CPPEditorEditable *cppEditable = qobject_cast<CPPEditorEditable *>(editor)) {
        if (m_cppEditor) {
            m_cppEditor = static_cast<CPPEditor *>(cppEditable->widget());
            return true;
        }
    } else if (!m_cppEditor) {
        return true;
    }
    return false;
}

void CppTypeHierarchyWidget::perform()
{
    if (!m_cppEditor)
        return;

    m_model->clear();

    CppElementEvaluator evaluator(m_cppEditor);
    evaluator.setLookupBaseClasses(true);
    QSharedPointer<CppElement> cppElement = evaluator.identifyCppElement();
    if (!cppElement.isNull()) {
        CppElement *element = cppElement.data();
        if (CppClass *cppClass = dynamic_cast<CppClass *>(element))
            buildModel(*cppClass, m_model->invisibleRootItem());
    }
}

void CppTypeHierarchyWidget::buildModel(const CppClass &cppClass, QStandardItem *parent)
{
    CppTypeHierarchyItem *item = new CppTypeHierarchyItem(cppClass);
    parent->appendRow(item);

    // The delegate retrieves data from the item directly. This is to help size hint.
    const QString &display = cppClass.name() + cppClass.qualifiedName();
    m_model->setData(m_model->indexFromItem(item), display, Qt::DisplayRole);
    m_model->setData(m_model->indexFromItem(item), cppClass.icon(), Qt::DecorationRole);

    foreach (const CppClass &cppBase, cppClass.bases())
        buildModel(cppBase, item);

    m_treeView->expand(m_model->indexFromItem(item));
}

void CppTypeHierarchyWidget::onItemClicked(const QModelIndex &index)
{
    if (QStandardItem *item = m_model->itemFromIndex(index)) {
        CppTypeHierarchyItem *cppItem = static_cast<CppTypeHierarchyItem *>(item);
        m_cppEditor->openLink(cppItem->cppClass().link());
    }
}

// CppTypeHierarchyStackedWidget
CppTypeHierarchyStackedWidget::CppTypeHierarchyStackedWidget(QWidget *parent) :
    QStackedWidget(parent),
    m_typeHiearchyWidgetInstance(
        new CppTypeHierarchyWidget(Core::EditorManager::instance()->currentEditor()))
{
    addWidget(m_typeHiearchyWidgetInstance);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(editorChanged(Core::IEditor*)));
}

CppTypeHierarchyStackedWidget::~CppTypeHierarchyStackedWidget()
{
    delete m_typeHiearchyWidgetInstance;
}

void CppTypeHierarchyStackedWidget::editorChanged(Core::IEditor *editor)
{
    if (!m_typeHiearchyWidgetInstance->handleEditorChange(editor)) {
        CppTypeHierarchyWidget *replacement = new CppTypeHierarchyWidget(editor);
        removeWidget(m_typeHiearchyWidgetInstance);
        m_typeHiearchyWidgetInstance->deleteLater();
        m_typeHiearchyWidgetInstance = replacement;
        addWidget(m_typeHiearchyWidgetInstance);
    }
}

// CppTypeHierarchyFactory
CppTypeHierarchyFactory::CppTypeHierarchyFactory()
{}

CppTypeHierarchyFactory::~CppTypeHierarchyFactory()
{}

QString CppTypeHierarchyFactory::displayName() const
{
    return tr("Type Hierarchy");
}

QString CppTypeHierarchyFactory::id() const
{
    return QLatin1String(Constants::TYPE_HIERARCHY_ID);
}

QKeySequence CppTypeHierarchyFactory::activationSequence() const
{
    return QKeySequence();
}

Core::NavigationView CppTypeHierarchyFactory::createWidget()
{
    CppTypeHierarchyStackedWidget *w = new CppTypeHierarchyStackedWidget;
    static_cast<CppTypeHierarchyWidget *>(w->currentWidget())->perform();
    Core::NavigationView navigationView;
    navigationView.widget = w;
    return navigationView;
}
