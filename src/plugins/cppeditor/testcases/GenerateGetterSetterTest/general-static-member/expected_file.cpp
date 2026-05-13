class Something
{
    static int m_member;

public:
    static int member();
    static void setMember(int member);
};

int Something::member()
{
    return m_member;
}

void Something::setMember(int member)
{
    m_member = member;
}
