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

#include "cppeditoroutline.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cpptoolssettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cplusplus/OverviewModel.h>
#include <utils/treeviewcombobox.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QTimer>

/*!
    \class CppTools::CppEditorOutline
    \brief A helper class that provides the outline model and widget,
           e.g. for the editor's tool bar.

    The caller is responsible for deleting the widget returned by widget().
 */

enum { UpdateOutlineIntervalInMs = 500 };

namespace {

class OverviewProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    OverviewProxyModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent)
        : QSortFilterProxyModel(parent)
        , m_sourceModel(sourceModel)
    {
        setSourceModel(m_sourceModel);
    }

    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
    {
        // Ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
        const QModelIndex sourceIndex = m_sourceModel->index(sourceRow, 0, sourceParent);
        CPlusPlus::Symbol *symbol = m_sourceModel->symbolFromIndex(sourceIndex);
        if (symbol && symbol->isGenerated())
            return false;

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
private:
    CPlusPlus::OverviewModel *m_sourceModel;
};

QTimer *newSingleShotTimer(QObject *parent, int msInternal, const QString &objectName)
{
    QTimer *timer = new QTimer(parent);
    timer->setObjectName(objectName);
    timer->setSingleShot(true);
    timer->setInterval(msInternal);
    return timer;
}

} // anonymous namespace

namespace CppTools {

CppEditorOutline::CppEditorOutline(TextEditor::TextEditorWidget *editorWidget)
    : QObject(editorWidget)
    , m_editorWidget(editorWidget)
    , m_combo(new Utils::TreeViewComboBox)
    , m_model(new CPlusPlus::OverviewModel(this))
    , m_proxyModel(new OverviewProxyModel(m_model, this))
{
    // Set up proxy model
    if (CppTools::CppToolsSettings::instance()->sortedEditorDocumentOutline())
        m_proxyModel->sort(0, Qt::AscendingOrder);
    else
        m_proxyModel->sort(-1, Qt::AscendingOrder); // don't sort yet, but set column for sortedOutline()
    m_proxyModel->setDynamicSortFilter(true);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    // Set up combo box
    m_combo->setModel(m_proxyModel);

    m_combo->setMinimumContentsLength(22);
    QSizePolicy policy = m_combo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_combo->setSizePolicy(policy);
    m_combo->setMaxVisibleItems(40);

    m_combo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_sortAction = new QAction(tr("Sort Alphabetically"), m_combo);
    m_sortAction->setCheckable(true);
    m_sortAction->setChecked(isSorted());
    connect(m_sortAction, &QAction::toggled,
            CppTools::CppToolsSettings::instance(),
            &CppTools::CppToolsSettings::setSortedEditorDocumentOutline);
    m_combo->addAction(m_sortAction);

    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, &CppEditorOutline::gotoSymbolInEditor);
    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CppEditorOutline::updateToolTip);

    // Set up timers
    m_updateTimer = newSingleShotTimer(this, UpdateOutlineIntervalInMs,
                                       QLatin1String("CppEditorOutline::m_updateTimer"));
    connect(m_updateTimer, &QTimer::timeout, this, &CppEditorOutline::updateNow);

    m_updateIndexTimer = newSingleShotTimer(this, UpdateOutlineIntervalInMs,
                                            QLatin1String("CppEditorOutline::m_updateIndexTimer"));
    connect(m_updateIndexTimer, &QTimer::timeout, this, &CppEditorOutline::updateIndexNow);
}

void CppEditorOutline::update()
{
    m_updateTimer->start();
}

bool CppEditorOutline::isSorted() const
{
    return m_proxyModel->sortColumn() == 0;
}

void CppEditorOutline::setSorted(bool sort)
{
    if (sort != isSorted()) {
        if (sort)
            m_proxyModel->sort(0, Qt::AscendingOrder);
        else
            m_proxyModel->sort(-1, Qt::AscendingOrder);
        {
            QSignalBlocker blocker(m_sortAction);
            m_sortAction->setChecked(m_proxyModel->sortColumn() == 0);
        }
        updateIndexNow();
    }
}

CPlusPlus::OverviewModel *CppEditorOutline::model() const
{
    return m_model;
}

QModelIndex CppEditorOutline::modelIndex()
{
    if (!m_modelIndex.isValid()) {
        int line = 0, column = 0;
        m_editorWidget->convertPosition(m_editorWidget->position(), &line, &column);
        m_modelIndex = indexForPosition(line, column);
        emit modelIndexChanged(m_modelIndex);
    }

    return m_modelIndex;
}

QWidget *CppEditorOutline::widget() const
{
    return m_combo;
}

void CppEditorOutline::updateNow()
{
    CppTools::CppModelManager *cmmi = CppTools::CppModelManager::instance();
    const CPlusPlus::Snapshot snapshot = cmmi->snapshot();
    const QString filePath = m_editorWidget->textDocument()->filePath().toString();
    CPlusPlus::Document::Ptr document = snapshot.document(filePath);
    if (!document)
        return;

    if (document->editorRevision() != (unsigned) m_editorWidget->document()->revision()) {
        m_updateTimer->start();
        return;
    }

    m_model->rebuild(document);

    m_combo->view()->expandAll();
    updateIndexNow();
}

void CppEditorOutline::updateIndex()
{
    m_updateIndexTimer->start();
}

void CppEditorOutline::updateIndexNow()
{
    if (!m_model->document())
        return;

    const unsigned revision = m_editorWidget->document()->revision();
    if (m_model->document()->editorRevision() != revision) {
        m_updateIndexTimer->start();
        return;
    }

    m_updateIndexTimer->stop();

    m_modelIndex = QModelIndex(); //invalidate
    QModelIndex comboIndex = modelIndex();

    if (comboIndex.isValid()) {
        QSignalBlocker blocker(m_combo);
        m_combo->setCurrentIndex(m_proxyModel->mapFromSource(comboIndex));
        updateToolTip();
    }
}

void CppEditorOutline::updateToolTip()
{
    m_combo->setToolTip(m_combo->currentText());
}

void CppEditorOutline::gotoSymbolInEditor()
{
    const QModelIndex modelIndex = m_combo->view()->currentIndex();
    const QModelIndex sourceIndex = m_proxyModel->mapToSource(modelIndex);
    CPlusPlus::Symbol *symbol = m_model->symbolFromIndex(sourceIndex);
    if (!symbol)
        return;

    const TextEditor::TextEditorWidget::Link &link = CppTools::linkToSymbol(symbol);
    if (!link.hasValidTarget())
        return;

    Core::EditorManager::cutForwardNavigationHistory();
    Core::EditorManager::addCurrentPositionToNavigationHistory();
    m_editorWidget->gotoLine(link.targetLine, link.targetColumn, true, true);
    m_editorWidget->activateEditor();
}

QModelIndex CppEditorOutline::indexForPosition(int line, int column,
                                               const QModelIndex &rootIndex) const
{
    QModelIndex lastIndex = rootIndex;

    const int rowCount = m_model->rowCount(rootIndex);
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_model->index(row, 0, rootIndex);
        CPlusPlus::Symbol *symbol = m_model->symbolFromIndex(index);
        if (symbol && symbol->line() > unsigned(line))
            break;
        lastIndex = index;
    }

    if (lastIndex != rootIndex) {
        // recurse
        lastIndex = indexForPosition(line, column, lastIndex);
    }

    return lastIndex;
}

} // namespace CppTools

#include <cppeditoroutline.moc>
