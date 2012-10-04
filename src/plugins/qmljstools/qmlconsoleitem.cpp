#include "qmlconsoleitem.h"

namespace QmlJSTools {

///////////////////////////////////////////////////////////////////////
//
// QmlConsoleItem
//
///////////////////////////////////////////////////////////////////////

QmlConsoleItem::QmlConsoleItem(QmlConsoleItem *parent, QmlConsoleItem::ItemType itemType,
                               const QString &text)
    : m_parentItem(parent),
      itemType(itemType),
      line(-1)

{
    setText(text);
}

QmlConsoleItem::~QmlConsoleItem()
{
    qDeleteAll(m_childItems);
}

QmlConsoleItem *QmlConsoleItem::child(int number)
{
    return m_childItems.value(number);
}

int QmlConsoleItem::childCount() const
{
    return m_childItems.size();
}

int QmlConsoleItem::childNumber() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<QmlConsoleItem *>(this));

    return 0;
}

bool QmlConsoleItem::insertChildren(int position, int count)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QmlConsoleItem *item = new QmlConsoleItem(this, QmlConsoleItem::UndefinedType,
                                                  QString());
        m_childItems.insert(position, item);
    }

    return true;
}

void QmlConsoleItem::insertChild(QmlConsoleItem *item, bool sorted)
{
    if (!sorted) {
        m_childItems.insert(m_childItems.count(), item);
        return;
    }

    int i = 0;
    for (; i < m_childItems.count(); i++) {
        if (item->m_text < m_childItems[i]->m_text)
            break;
    }
    m_childItems.insert(i, item);
}

bool QmlConsoleItem::insertChild(int position, QmlConsoleItem *item)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    m_childItems.insert(position, item);

    return true;
}

QmlConsoleItem *QmlConsoleItem::parent()
{
    return m_parentItem;
}

bool QmlConsoleItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > m_childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete m_childItems.takeAt(position);

    return true;
}

bool QmlConsoleItem::detachChild(int position)
{
    if (position < 0 || position > m_childItems.size())
        return false;

    m_childItems.removeAt(position);

    return true;
}

void QmlConsoleItem::setText(const QString &text)
{
    m_text = text;
    for (int i = 0; i < m_text.length(); ++i) {
        if (m_text.at(i).isPunct())
            m_text.insert(++i, QChar(0x200b)); // ZERO WIDTH SPACE
    }
}

const QString &QmlConsoleItem::text() const
{
    return m_text;
}

} // QmlJSTools
