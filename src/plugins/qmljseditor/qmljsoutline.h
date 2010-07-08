#ifndef QMLJSOUTLINE_H
#define QMLJSOUTLINE_H

#include "qmljseditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <texteditor/ioutlinewidget.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>

#include <QtGui/QStandardItemModel>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

namespace Core {
class IEditor;
}

namespace QmlJS {
class Editor;
};

namespace QmlJSEditor {
namespace Internal {

class QmlOutlineModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum CustomRoles {
        SourceLocationRole = Qt::UserRole + 1
    };

    QmlOutlineModel(QObject *parent = 0);

    void startSync();

    QModelIndex enterElement(const QString &typeName, const QmlJS::AST::SourceLocation &location);
    void leaveElement();

    QModelIndex enterProperty(const QString &name, const QmlJS::AST::SourceLocation &location);
    void leaveProperty();

private:
    QStandardItem *enterNode(const QmlJS::AST::SourceLocation &location);
    void leaveNode();

    QStandardItem *parentItem();

    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons m_icons;
};

class QmlJSOutlineTreeView : public QTreeView
{
    Q_OBJECT
public:
    QmlJSOutlineTreeView(QWidget *parent = 0);
};


class QmlJSOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    QmlJSOutlineWidget(QWidget *parent = 0);

    void setEditor(QmlJSTextEditor *editor);

    // IOutlineWidget
    virtual void setCursorSynchronization(bool syncWithCursor);

private slots:
    void updateOutline(const QmlJSEditor::Internal::SemanticInfo &semanticInfo);
    void updateSelectionInTree();
    void updateSelectionInText(const QItemSelection &selection);

private:
    QModelIndex indexForPosition(const QModelIndex &rootIndex, int cursorPosition);
    bool offsetInsideLocation(quint32 offset, const QmlJS::AST::SourceLocation &location);
    bool syncCursor();

private:
    QmlJSOutlineTreeView *m_treeView;
    QAbstractItemModel *m_model;
    QWeakPointer<QmlJSTextEditor> m_editor;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
};

class QmlJSOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
    Q_OBJECT
public:
    bool supportsEditor(Core::IEditor *editor) const;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor);
};

} // namespace Internal
} // namespace QmlJSEditor

Q_DECLARE_METATYPE(QmlJS::AST::SourceLocation);

#endif // QMLJSOUTLINE_H
