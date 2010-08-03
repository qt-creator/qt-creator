#include "qmljscontextcrumblepath.h"

#include <QMouseEvent>
#include <QDebug>

namespace QmlJSInspector {
namespace Internal {

ContextCrumblePath::ContextCrumblePath(QWidget *parent)
    : CrumblePath(parent), m_isEmpty(true)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    updateContextPath(QStringList());
}

ContextCrumblePath::~ContextCrumblePath()
{

}

void ContextCrumblePath::updateContextPath(const QStringList &path)
{
    clear();
    foreach(const QString &pathPart, path) {
        pushElement(pathPart);
    }

    m_isEmpty = path.isEmpty();
    if (m_isEmpty) {
        pushElement(tr("[no context]"));
    }
}

bool ContextCrumblePath::isEmpty() const
{
    return m_isEmpty;
}

} // namespace Internal
} // namespace QmlJSInspector
