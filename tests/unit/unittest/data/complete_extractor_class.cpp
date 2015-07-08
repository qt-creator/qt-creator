class Class {};
struct Struct{};
union Union{};
typedef Class TypeDef;
template<class T> class TemplateClass{};
template<class T> class ClassTemplatePartialSpecialization;
template<class T> class ClassTemplatePartialSpecialization<T*>;









template<class TemplateTypeParameter, template<class> class TemplateTemplateParameter>
void function()
{

}
