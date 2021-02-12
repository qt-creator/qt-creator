struct S {};
void func(int i, int j);
void func(const S &s, int j);
void func(const S &s, int j, int k);

void g()
{
    func(S(), /* COMPLETE HERE */
}
