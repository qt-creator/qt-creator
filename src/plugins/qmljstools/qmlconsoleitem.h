#ifndef QMLCONSOLEITEM_H
#define QMLCONSOLEITEM_H

#include "qmljstools_global.h"

#include <QList>
#include <QString>

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QmlConsoleItem
{
public:
    enum ItemType
    {
        UndefinedType = 0x01, // Can be used for unknown and for Return values
        DebugType     = 0x02,
        WarningType   = 0x04,
        ErrorType     = 0x08,
        InputType     = 0x10,
        DefaultTypes  = InputType | UndefinedType
    };
    Q_DECLARE_FLAGS(ItemTypes, ItemType)

    QmlConsoleItem(QmlConsoleItem *parent,
                   QmlConsoleItem::ItemType type = QmlConsoleItem::UndefinedType,
                   const QString &data = QString());
    ~QmlConsoleItem();

    QmlConsoleItem *child(int number);
    int childCount() const;
    bool insertChildren(int position, int count);
    void insertChild(QmlConsoleItem *item, bool sorted);
    bool insertChild(int position, QmlConsoleItem *item);
    QmlConsoleItem *parent();
    bool removeChildren(int position, int count);
    bool detachChild(int position);
    int childNumber() const;
    void setText(const QString &text);
    const QString &text() const;

private:
    QmlConsoleItem *m_parentItem;
    QList<QmlConsoleItem *> m_childItems;
    QString m_text;

public:
    QmlConsoleItem::ItemType itemType;
    QString file;
    int line;
};

} // QmlJSTools

#endif // QMLCONSOLEITEM_H
