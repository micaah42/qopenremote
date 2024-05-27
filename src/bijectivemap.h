#ifndef BIJECTIVEMAP_H
#define BIJECTIVEMAP_H

#include <QMap>

template<class A, class B>
class BijectiveMap
{
public:
    const A &b2a(const B &b) const { return _b2a[b]; };
    A &b2a(const B &b) { return _b2a[b]; };

    const B &a2b(const A &a) const { return _a2b[a]; };
    B &a2b(const A &a) { return _a2b[a]; };

    void insert(const A &a, const B &b)
    {
        _a2b.insert(a, b);
        _b2a.insert(b, a);
    };

    void remove(const A &a, const B &b)
    {
        _a2b.remove(a);
        _b2a.remove(b);
    };

    const QMap<A, B> &a2bMap() const { return _a2b; };
    const QMap<B, A> &b2aMap() const { return _b2a; };

protected:
    // changing a single map can break validity!

    // [[deprecated]] QMap<A, B> &a2bMap() { return _a2b; };
    // [[deprecated]] QMap<A, B> &b2aMap() { return _b2a; };

private:
    QMap<A, B> _a2b;
    QMap<B, A> _b2a;
};
#endif // BIJECTIVEMAP_H
