#pragma once

// THIS IS HERE BECAUSE CERES DOESN'T USE SHARED POINTERS
// What this does is it protects the functor from
// ceres wanting to delete all the residual blocks
// This gives ceres something to delete, but protects the actual
// functor from being deleted

namespace salsa
{

template<typename Functor>
struct FunctorShield
{
    FunctorShield(const Functor* functor) :
        ptr(functor)
    {}

    template <typename ... T>
    bool operator()(T... args) const
    {
        return ptr->operator ()(args...);
    }
    template <typename ... T>
    bool Evaluate(T... args) const
    {
        return ptr->Evaluate(args...);
    }
    const Functor* ptr;
};

}
