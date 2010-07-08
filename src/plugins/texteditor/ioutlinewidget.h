#ifndef IOUTLINEWIDGET_H
#define IOUTLINEWIDGET_H

#include <texteditor/texteditor_global.h>
#include <QtGui/QWidget>

namespace Core {
class IEditor;
}

namespace TextEditor {

class TEXTEDITOR_EXPORT IOutlineWidget : public QWidget
{
    Q_OBJECT
public:
    IOutlineWidget(QWidget *parent = 0) : QWidget(parent) {}

    virtual void setCursorSynchronization(bool syncWithCursor) = 0;
};

class TEXTEDITOR_EXPORT IOutlineWidgetFactory : public QObject {
    Q_OBJECT
public:
    virtual bool supportsEditor(Core::IEditor *editor) const = 0;
    virtual IOutlineWidget *createWidget(Core::IEditor *editor) = 0;
};

} // namespace TextEditor

#endif // IOUTLINEWIDGET_H
