void func()
{
    std::unique_ptr<int> other;
    std::unique_ptr<int> @p((std::move(other)));
}
