
class Base {
public:

    int zebra;
};

template<typename T>
class Child : public Base {
public:
    int tiger;

    void doit()
    {
        tiger = zebra + 42; // No problem seeing zebra
    }
};

template<typename T>
class GrandChild : public Child<T> {
public:

    int lion;

    void doneit()
    {
        // Compiler complains it can't see zebra or tiger
        // unless I qualify names
        lion = Child<T>::zebra + Child<T>::tiger; 
    }
};

int main()
{
    GrandChild<int> audry;

    audry.lion = 42;
    audry.tiger = 2020;
    audry.zebra = 1902;

    audry.doneit();
}

