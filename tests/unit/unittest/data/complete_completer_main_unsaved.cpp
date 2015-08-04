#include "complete_forwarding_header_2.h"

void Function()
{

}

class Foo;
void FunctionWithArguments(int i, char *c, const Foo &ref)
{

}

void UnsavedFunction()
{

}

#define Macro

int GlobalVariableInUnsavedFile;

void f()
{
 int VariableInUnsavedFile;


}
