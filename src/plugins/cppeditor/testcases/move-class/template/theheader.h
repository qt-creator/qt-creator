enum SomeEnum { E };
template<typename T> class TheClass
{
    T defaultValue() const;
};

template<typename T> inline T TheClass<T>::defaultValue() const { return T(); }
