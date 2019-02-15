#pragma once

#define HEADER_DEFINE

void HeaderFunction();
void HeaderFunctionReference();
void HeaderFunctionReferenceInMainFile();

class Class
{
public:
    int Member = 20;
    int MemberReference = 10;

    void InlineHeaderMemberFunction()
    {
        int FunctionLocalVariable = 90;
        MemberReference = 30;
        HeaderFunctionReference();
    }
};

