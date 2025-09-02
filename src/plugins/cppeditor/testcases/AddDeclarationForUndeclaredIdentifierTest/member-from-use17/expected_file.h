class C {
public:
    int value() const { return valueInternal(); }
private:
    int valueInternal() const;
};
