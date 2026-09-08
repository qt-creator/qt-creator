int otherHelper(int x) { return x * 2; }

int helper(int a)
{
    return otherHelper(a);
}

int user()
{
    return hel@per(5);
}
