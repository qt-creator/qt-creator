#ifndef SHAREDPOINTER_H
#define SHAREDPOINTER_H

#include <botan/build.h>

#ifdef BOTAN_USE_QT_SHARED_POINTER
    #include <QtCore/QSharedPointer>
#elif defined(BOTAN_USE_STD_TR1)
    #include <tr1/memory>
#elif defined(BOTAN_USE_BOOST_TR1)
    #include <boost/tr1/memory.hpp>
#else
  #error "Please choose a TR1 implementation in build.h"
#endif

namespace Botan {

template <typename T> class SharedPointer
{
public:
#ifdef BOTAN_USE_QT_SHARED_POINTER
    typedef QSharedPointer<T> Ptr;
#else
    typedef std::tr1::shared_ptr<T> Ptr;
#endif

    SharedPointer() {}
    SharedPointer(T *rawPtr) : m_ptr(rawPtr) {}
    template<typename X> SharedPointer(const SharedPointer<X> &other)
        : m_ptr(other.internalPtr()) {}
    template<typename X> SharedPointer<T> &operator=(const SharedPointer<X> &other)
    {
        m_ptr = other.internalPtr();
        return *this;
    }

    const Ptr &internalPtr() const { return m_ptr; }

    T *get() const
    {
#ifdef BOTAN_USE_QT_SHARED_POINTER
        return m_ptr.data();
#else
        return m_ptr.get();
#endif
    }

    void reset(T *rawPtr)
    {
#ifdef BOTAN_USE_QT_SHARED_POINTER
        m_ptr = QSharedPointer<T>(rawPtr);
#else
        m_ptr.reset(rawPtr);
#endif
    }

    T &operator* () const { return *m_ptr; }
    T *operator-> () const { return m_ptr.operator->(); }
    void swap(SharedPointer<T> &other) { m_ptr.swap(other.m_ptr); }

private:
    Ptr m_ptr;
};

template <typename X, typename Y>
inline bool operator==(const SharedPointer<X> & ptr1, const SharedPointer<Y> & ptr2)
{
    return ptr1.get() == ptr2.get();
}

template <typename X, typename Y>
inline bool operator!=(const SharedPointer<X> & ptr1, const SharedPointer<Y> & ptr2)
{
    return !(ptr1 == ptr2);
}

} // namespace Botan

#endif // SHAREDPOINTER_H
