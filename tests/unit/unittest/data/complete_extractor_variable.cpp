void function(int Parameter)
{
    int Var = 0;

}

void function2()
{
    int Var = 0;
    auto Lambda = [&Var]()
    {

    };
}

class Class {
    int Field;

    void function() {

    }
};

template <int NonTypeTemplateParameter>
void function3() {}

#define MacroDefinition


void function4()
{
#ifdef ArgumentDefinition
    int ArgumentDefinitionVariable;
#endif

}
