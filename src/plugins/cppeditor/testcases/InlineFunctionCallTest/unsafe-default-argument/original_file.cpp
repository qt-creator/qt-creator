int getDefault();

int helper(int a, int b = getDefault()) { return a + b; }

int user()
{
    return hel@per(3);
}
