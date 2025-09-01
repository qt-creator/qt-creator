template<typename T> concept IsPointer = requires(T p) { *p; };
template<IsPointer T> void* func(T p) { return p; }
void *func2(IsPointer auto p)
{
    return p;
}
