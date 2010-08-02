#include "qmljscontextcrumblepath.h"

#include <QMouseEvent>
#include <QDebug>

namespace QmlJSInspector {
namespace Internal {

ContextCrumblePath::ContextCrumblePath(QWidget *parent)
    : CrumblePath(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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
}

} // namespace Internal
} // namespace QmlJSInspector
