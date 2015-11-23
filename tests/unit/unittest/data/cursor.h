


namespace Namespace
{
class SuperClass;
/**
 * A brief comment
 */
class SuperClass
{
    SuperClass() = default;
    SuperClass(int x) noexcept;
    int Method();
    virtual int VirtualMethod(int z);
    virtual int AbstractVirtualMethod(int z) = 0;
    bool ConstMethod() const;
    static void StaticMethod();
    operator int() const;
    int operator ++() const;
    ~SuperClass();

private:
    int y;
};
}

struct Struct final
{
    virtual void FinalVirtualMethod() final;
};

union Union
{

};

struct NonFinalStruct
{
    virtual void FinalVirtualMethod() final;
    void function();
};
