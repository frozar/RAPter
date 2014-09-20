#ifndef SAMPLER_H
#define SAMPLER_H

#include "primitive.h"

namespace InputGen{


template <typename _Scalar, template <class> class _DisplayFunctor, class _Primitive>
class VisibleSampler{
protected:
    typedef _Scalar Scalar;
    typedef _Primitive Primitive;
    typedef _DisplayFunctor<_Scalar> DisplayFunctor;

public:
    virtual void display() const = 0;
    virtual VisibleSampler<_Scalar, _DisplayFunctor, _Primitive>* copy() = 0;

};

//! Basic sampler, sampling the primitive regularly
template <typename _Scalar,
          template <class> class _DisplayFunctor,
          class _Primitive>
struct PrimitiveSampler : public VisibleSampler<
        _Scalar,
        _DisplayFunctor,
        _Primitive>{
    typedef _Scalar Scalar;
    typedef _Primitive Primitive;
    typedef _DisplayFunctor<_Scalar> DisplayFunctor;

    ///// parameters
    Scalar spacing;


    ///// copy constructor
    /// inline
    PrimitiveSampler(PrimitiveSampler<_Scalar, _DisplayFunctor, Primitive>* other)
        : spacing(other->spacing)
    {}
    PrimitiveSampler()
        : spacing(1.)
    {}

    ///// processing
    template <class SampleContainer, class PrimitiveContainer>
    inline void generateSamples(      SampleContainer&    scontainer,
                                const PrimitiveContainer& pcontainer);

    virtual void display() const { }

    virtual VisibleSampler<_Scalar, _DisplayFunctor, _Primitive>* copy(){
        return new PrimitiveSampler<_Scalar, _DisplayFunctor, _Primitive>(this);
    }
};

//! Basic sampler, sampling the primitive regularly
template <typename _Scalar,
          template <class> class _DisplayFunctor,
          class _Primitive>
struct PonctualSampler : public VisibleSampler<
        _Scalar,
        _DisplayFunctor,
        _Primitive>{
    typedef _Scalar Scalar;
    typedef _Primitive Primitive;
    typedef typename _Primitive::vec vec;
    typedef _DisplayFunctor<_Scalar> DisplayFunctor;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    ///// parameters
    int nbSamples;  //
    bool occlusion; //! <\brief enable occlusions
    vec pos;



    ///// copy constructor
    /// inline
    PonctualSampler(PonctualSampler<_Scalar, _DisplayFunctor, _Primitive>* other)
        : nbSamples(other->nbSamples),
          occlusion(other->occlusion),
          pos(other->vec)
    {}
    PonctualSampler()
        : nbSamples(1), occlusion(true), pos(vec::Zero())
    {}

    ///// processing
    template <class SampleContainer, class PrimitiveContainer>
    inline void generateSamples(      SampleContainer&    scontainer,
                                const PrimitiveContainer& pcontainer);

    virtual void display() const {
        DisplayFunctor dfunctor;
    }

    virtual VisibleSampler<_Scalar, _DisplayFunctor, _Primitive>* copy(){
        return new PonctualSampler<_Scalar, _DisplayFunctor, _Primitive>(this);
    }
};


}

#include "impl/sampler.hpp"

#endif // SAMPLER_H
