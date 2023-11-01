// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppeditoroutline.h"

#include "cppeditordocument.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppoutlinemodel.h"
#include "cpptoolssettings.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/treeviewcombobox.h>

#include <QAction>
#include <QSortFilterProxyModel>
#include <QTimer>

/*!
    \class CppEditor::CppEditorOutline
    \brief A helper class that provides the outline model and widget,
           e.g. for the editor's tool bar.

    The caller is responsible for deleting the widget returned by widget().
 */

enum { UpdateOutlineIntervalInMs = 500 };

namespace {

class OutlineProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    OutlineProxyModel(CppEditor::Internal::OutlineModel &sourceModel, QObject *parent)
        : QSortFilterProxyModel(parent)
        , m_sourceModel(sourceModel)
    {
    }

    bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const override
    {
        // Ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
        const QModelIndex sourceIndex = m_sourceModel.index(sourceRow, 0, sourceParent);
        if (m_sourceModel.isGenerated(sourceIndex))
            return false;

        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }
private:
    CppEditor::Internal::OutlineModel &m_sourceModel;
};

QTimer *newSingleShotTimer(QObject *parent, int msInternal, const QString &objectName)
{
    auto *timer = new QTimer(parent);
    timer->setObjectName(objectName);
    timer->setSingleShot(true);
    timer->setInterval(msInternal);
    return timer;
}

} // anonymous namespace

namespace CppEditor::Internal {

CppEditorOutline::CppEditorOutline(CppEditorWidget *editorWidget)
    : QObject(editorWidget)
    , m_editorWidget(editorWidget)
    , m_combo(new Utils::TreeViewComboBox)
{
    m_model = &editorWidget->cppEditorDocument()->outlineModel();
    m_proxyModel = new OutlineProxyModel(*m_model, this);
    m_proxyModel->setSourceModel(m_model);

    // Set up proxy model
    if (CppToolsSettings::sortedEditorDocumentOutline())
        m_proxyModel->sort(0, Qt::AscendingOrder);
    else
        m_proxyModel->sort(-1, Qt::AscendingOrder); // don't sort yet, but set column for sortedOutline()
    m_proxyModel->setDynamicSortFilter(true);

    // Set up combo box
    m_combo->setModel(m_proxyModel);

    m_combo->setMinimumContentsLength(13);
    QSizePolicy policy = m_combo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_combo->setSizePolicy(policy);
    m_combo->setMaxVisibleItems(40);

    m_combo->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_sortAction = new QAction(Tr::tr("Sort Alphabetically"), m_combo);
    m_sortAction->setCheckable(true);
    m_sortAction->setChecked(isSorted());
    connect(m_sortAction, &QAction::toggled,
            CppToolsSettings::instance(),
            &CppToolsSettings::setSortedEditorDocumentOutline);
    m_combo->addAction(m_sortAction);

    connect(m_combo, &QComboBox::activated, this, &CppEditorOutline::gotoSymbolInEditor);
    connect(m_combo, &QComboBox::currentIndexChanged, this, &CppEditorOutline::updateToolTip);

    connect(m_model, &OutlineModel::modelReset, this, &CppEditorOutline::updateNow);
    // Set up timers
    m_updateIndexTimer = newSingleShotTimer(this, UpdateOutlineIntervalInMs,
                                            QLatin1String("CppEditorOutline::m_updateIndexTimer"));
    connect(m_updateIndexTimer, &QTimer::timeout, this, &CppEditorOutline::updateIndexNow);
}

bool CppEditorOutline::isSorted() const
{
    return m_proxyModel->sortColumn() == 0;
}

QWidget *CppEditorOutline::widget() const
{
    return m_combo;
}

QSharedPointer<CPlusPlus::Document> getDocument(const Utils::FilePath &filePath)
{
    const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    return snapshot.document(filePath);
}

void CppEditorOutline::updateNow()
{
    m_combo->view()->expandAll();
    updateIndexNow();
}

void CppEditorOutline::updateIndex()
{
    m_updateIndexTimer->start();
}

void CppEditorOutline::updateIndexNow()
{
    if (m_model->editorRevision() != m_editorWidget->document()->revision()) {
        m_editorWidget->cppEditorDocument()->updateOutline();
        return;
    }

    m_updateIndexTimer->stop();

    int line = 0, column = 0;
    m_editorWidget->convertPosition(m_editorWidget->position(), &line, &column);
    QModelIndex comboIndex = m_model->indexForPosition(line, column);

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

    const Utils::Link link = m_model->linkFromIndex(sourceIndex);
    if (!link.hasValidTarget())
        return;

    Core::EditorManager::cutForwardNavigationHistory();
    Core::EditorManager::addCurrentPositionToNavigationHistory();
    m_editorWidget->gotoLine(link.targetLine, link.targetColumn, true, true);
    emit m_editorWidget->activateEditor();
}

} // namespace CppEditor::Internal

#include <cppeditoroutline.moc>
