// Left
template<typename... Args>
bool all(Args... args) { return (... && args); }

// Right
template<typename... Args>
int sum(Args&&... args)
{
    return (args + ...);
}

// Binary
template<typename... Args>
int mult(Args... args)
{
    return (1 * ... * (args));
}
