#ifndef CPLUSPLUS_ALREADYCONSIDEREDCLASSCONTAINER_H
#define CPLUSPLUS_ALREADYCONSIDEREDCLASSCONTAINER_H

#include <QSet>

namespace CPlusPlus {

template<typename T>
class AlreadyConsideredClassContainer
{
public:
    AlreadyConsideredClassContainer() : _class(0) {}
    void insert(const T *item)
    {
        if (_container.isEmpty())
            _class = item;
        _container.insert(item);
    }
    bool contains(const T *item)
    {
        if (_container.contains(item))
            return true;

        foreach (const T *existingItem, _container) {
            if (existingItem->isEqualTo(item))
                return true;
        }

        return false;
    }

    void clear(const T *item)
    {
        if (_class != item || _container.size() == 1)
            _container.clear();
    }

private:
    QSet<const T *> _container;
    const T * _class;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_ALREADYCONSIDEREDCLASSCONTAINER_H
