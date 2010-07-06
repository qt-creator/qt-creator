#ifndef QMLJSICONTEXTPANE_H
#define QMLJSICONTEXTPANE_H

#include <QObject>
#include "qmljs_global.h"
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>


namespace TextEditor {

class BaseTextEditorEditable;

} //TextEditor

namespace QmlJS {


class QMLJS_EXPORT IContextPane : public QObject
{
     Q_OBJECT

public:
    IContextPane(QObject *parent = 0) : QObject(parent) {}
    virtual ~IContextPane() {}
    virtual void apply(TextEditor::BaseTextEditorEditable *editor, Document::Ptr doc, AST::Node *node, bool update) = 0;
    virtual void setEnabled(bool) = 0;
};

} // namespace QmlJS

#endif // QMLJSICONTEXTPANE_H
