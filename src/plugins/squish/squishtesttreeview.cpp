// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishtesttreeview.h"

#include "squishconstants.h"
#include "squishfilehandler.h"
#include "squishsettings.h"
#include "squishtesttreemodel.h"
#include "suiteconf.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Squish {
namespace Internal {

SquishTestTreeView::SquishTestTreeView(QWidget *parent)
    : Utils::NavigationTreeView(parent)
    , m_context(new Core::IContext(this))
{
    setExpandsOnDoubleClick(false);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::SQUISH_CONTEXT));
    Core::ICore::addContextObject(m_context);
}

void SquishTestTreeView::resizeEvent(QResizeEvent *event)
{
    // override derived behavior of Utils::NavigationTreeView as we have more than 1 column
    Utils::TreeView::resizeEvent(event);
}

void SquishTestTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index.column() > 0 && index.column() < 3) {
            int type = index.data(TypeRole).toInt();
            if (type == SquishTestTreeItem::SquishSuite
                || type == SquishTestTreeItem::SquishTestCase) {
                m_lastMousePressedIndex = index;
            }
        }
    }
    QTreeView::mousePressEvent(event);
}

void SquishTestTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index == m_lastMousePressedIndex) {
            int type = index.data(TypeRole).toInt();
            if (type == SquishTestTreeItem::SquishSuite) {
                if (index.column() == 1)
                    emit runTestSuite(index.data(DisplayNameRole).toString());
                else if (index.column() == 2)
                    emit openObjectsMap(index.data(DisplayNameRole).toString());
            } else {
                const QModelIndex &suiteIndex = index.parent();
                if (suiteIndex.isValid()) {
                    if (index.column() == 1) {
                        emit runTestCase(suiteIndex.data(DisplayNameRole).toString(),
                                         index.data(DisplayNameRole).toString());
                    } else if (index.column() == 2) {
                        emit recordTestCase(suiteIndex.data(DisplayNameRole).toString(),
                                            index.data(DisplayNameRole).toString());
                    }
                }
            }
        }
    }
    QTreeView::mouseReleaseEvent(event);
}

/****************************** SquishTestTreeItemDelegate *************************************/

SquishTestTreeItemDelegate::SquishTestTreeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void SquishTestTreeItemDelegate::paint(QPainter *painter,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex &idx) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, idx);

    // elide first column if necessary
    if (idx.column() == 0)
        opt.textElideMode = Qt::ElideMiddle;

    // display disabled items as enabled
    if (idx.flags() == Qt::NoItemFlags)
        opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::Text));

    QStyledItemDelegate::paint(painter, opt, idx);
}

QSize SquishTestTreeItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                           const QModelIndex &idx) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, idx);

    // elide first column if necessary
    if (idx.column() == 0)
        opt.textElideMode = Qt::ElideMiddle;
    return QStyledItemDelegate::sizeHint(opt, idx);
}

QWidget *SquishTestTreeItemDelegate::createEditor(QWidget *parent,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &index) const
{
    Q_UNUSED(option)
    QTC_ASSERT(parent, return nullptr);
    QTC_ASSERT(index.isValid(), return nullptr);
    auto model = static_cast<const SquishTestTreeSortModel *>(index.model());
    QTC_ASSERT(model, return nullptr);
    auto srcModel = static_cast<SquishTestTreeModel *>(model->sourceModel());

    const SquishTestTreeItem *suite = srcModel->itemForIndex(model->mapToSource(index.parent()));
    SquishTestTreeItem *testCaseItem = srcModel->itemForIndex(model->mapToSource(index));
    const SuiteConf suiteConf = SuiteConf::readSuiteConf(suite->filePath());
    const QStringList inUse = suiteConf.usedTestCases();
    Utils::FancyLineEdit *editor = new Utils::FancyLineEdit(parent);
    editor->setValidationFunction([inUse](Utils::FancyLineEdit *edit, QString *) {
        static const QRegularExpression validFileName("^[-a-zA-Z0-9_$. ]+$");
        QString testName = edit->text();
        if (!testName.startsWith("tst_"))
            testName.prepend("tst_");
        return validFileName.match(testName).hasMatch() && !inUse.contains(testName);
    });

    connect(this, &QStyledItemDelegate::closeEditor,
            editor, [srcModel, testCaseItem](QWidget *, EndEditHint hint) {
        QTC_ASSERT(srcModel, return);
        QTC_ASSERT(testCaseItem, return);
        if (hint != QAbstractItemDelegate::RevertModelCache)
            return;
        srcModel->destroyItem(testCaseItem);
    });
    return editor;
}

void SquishTestTreeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QTC_ASSERT(editor, return);
    QTC_ASSERT(index.isValid(), return);

    static_cast<Utils::FancyLineEdit *>(editor)->setText(index.data().toString());
}

static bool copyScriptTemplates(const SuiteConf &suiteConf, const Utils::FilePath &destination)
{
    Utils::expected_str<void> result = destination.ensureWritableDir();
    QTC_ASSERT_EXPECTED(result, return false);

    const bool scripted = suiteConf.objectMapStyle() == "script";
    const QString extension = suiteConf.scriptExtension();
    const QString testStr = scripted ? QString("script_som_template") : QString("script_template");

    const Utils::FilePath scripts = settings().scriptsPath(suiteConf.language());
    const Utils::FilePath test = scripts.pathAppended(testStr + extension);
    const Utils::FilePath testFile = destination.pathAppended("test" + extension);
    QTC_ASSERT(!testFile.exists(), return false);
    result = test.copyFile(testFile);
    QTC_ASSERT_EXPECTED(result, return false);

    if (scripted)
        return suiteConf.ensureObjectMapExists();
    return true;
}

void SquishTestTreeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                              const QModelIndex &index) const
{
    QTC_ASSERT(editor, return);
    QTC_ASSERT(model, return);
    QTC_ASSERT(index.isValid(), return);

    auto sortModel = static_cast<SquishTestTreeSortModel *>(model);
    auto sourceModel = static_cast<SquishTestTreeModel *>(sortModel->sourceModel());
    auto lineEdit = static_cast<Utils::FancyLineEdit *>(editor);
    auto removeFormerlyAdded = [sortModel, sourceModel, &index] {
        auto item = sourceModel->itemForIndex(sortModel->mapToSource(index));
        QTC_ASSERT(item, return);
        sourceModel->destroyItem(item);
    };

    if (!lineEdit->isValid()) {
        // remove the formerly added again
        removeFormerlyAdded();
        return;
    }

    QString chosenName = lineEdit->text();
    if (!chosenName.startsWith("tst_"))
        chosenName.prepend("tst_");

    const QModelIndex parent = index.parent();
    SquishTestTreeItem *suiteItem = sourceModel->itemForIndex(sortModel->mapToSource(parent));
    const auto suiteConfPath = suiteItem->filePath();
    SuiteConf suiteConf = SuiteConf::readSuiteConf(suiteConfPath);
    const Utils::FilePath destination = suiteConfPath.parentDir().pathAppended(chosenName);
    bool ok = copyScriptTemplates(suiteConf, destination);
    QTC_ASSERT(ok, removeFormerlyAdded(); return);

    suiteConf.addTestCase(chosenName);
    ok = suiteConf.write();
    QTC_ASSERT(ok, removeFormerlyAdded(); return);
    SquishFileHandler::instance()->openTestSuite(suiteConfPath, true);

    Core::EditorManager::openEditor(destination.pathAppended("test" + suiteConf.scriptExtension()));
}

} // namespace Internal
} // namespace Squish
