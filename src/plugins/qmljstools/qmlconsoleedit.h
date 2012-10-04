#ifndef QMLCONSOLEEDIT_H
#define QMLCONSOLEEDIT_H

#include "qmljsinterpreter.h"

#include <QTextEdit>
#include <QModelIndex>

namespace QmlJSTools {
namespace Internal {

class QmlConsoleEdit : public QTextEdit
{
    Q_OBJECT
public:
    QmlConsoleEdit(const QModelIndex &index, QWidget *parent);

    QString getCurrentScript() const;

protected:
    void keyPressEvent(QKeyEvent *e);
    void contextMenuEvent(QContextMenuEvent *event);
    void focusOutEvent(QFocusEvent *e);

signals:
    void editingFinished();

protected:
    void handleUpKey();
    void handleDownKey();

    void replaceCurrentScript(const QString &script);

private:
    QModelIndex m_historyIndex;
    QString m_cachedScript;
    QImage m_prompt;
    int m_startOfEditableArea;
    QmlJSInterpreter m_interpreter;
};

} // QmlJSTools
} // Internal

#endif // QMLCONSOLEEDIT_H
