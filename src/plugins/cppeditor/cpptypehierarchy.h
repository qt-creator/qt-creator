// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/inavigationwidgetfactory.h>
#include <utils/futuresynchronizer.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QList>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QStackedLayout;
class QStandardItem;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorLinkLabel; }

namespace Utils {
class AnnotatedItemDelegate;
class NavigationTreeView;
class ProgressIndicator;
}

namespace CppEditor {
class CppEditorWidget;

namespace Internal {
class CppClass;
class CppElement;

class CppTypeHierarchyModel : public QStandardItemModel
{
    Q_OBJECT

public:
    CppTypeHierarchyModel(QObject *parent);

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
};

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget();

    void perform();

private slots:
    void displayHierarchy();

private:
    typedef QList<CppClass> CppClass::*HierarchyMember;
    void performFromExpression(const QString &expression, const Utils::FilePath &filePath);
    QStandardItem *buildHierarchy(const CppClass &cppClass, QStandardItem *parent,
                                  bool isRoot, HierarchyMember member);
    void showNoTypeHierarchyLabel();
    void showTypeHierarchy();
    void showProgress();
    void hideProgress();
    void clearTypeHierarchy();
    void onItemActivated(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);

    CppEditorWidget *m_cppEditor = nullptr;
    Utils::NavigationTreeView *m_treeView = nullptr;
    QWidget *m_hierarchyWidget = nullptr;
    QStackedLayout *m_stackLayout = nullptr;
    QStandardItemModel *m_model = nullptr;
    Utils::AnnotatedItemDelegate *m_delegate = nullptr;
    TextEditor::TextEditorLinkLabel *m_inspectedClass = nullptr;
    QLabel *m_infoLabel = nullptr;
    QFuture<QSharedPointer<CppElement>> m_future;
    QFutureWatcher<void> m_futureWatcher;
    Utils::FutureSynchronizer m_synchronizer;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    QString m_oldClass;
    bool m_showOldClass = false;
};

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CppTypeHierarchyFactory();

    Core::NavigationView createWidget()  override;
};

} // namespace Internal
} // namespace CppEditor
