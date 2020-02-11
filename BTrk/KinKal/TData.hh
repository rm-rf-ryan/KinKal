#ifndef KinKal_TData_hh
#define KinKal_TData_hh
//
//  Data payload to describing either parameters or weights
//  Parameters describe values and covariance of fit results and
//  describe a physical trajectory and the mathematical inverse of
//  weights.  Weights express constraints and are the mathematical
//  inverse of parameters.
//  used as part of the kinematic kalman fit
//
#include "BTrk/KinKal/TDataBase.hh"
#include "Math/SVector.h"
#include "Math/SMatrix.h"
#include <iostream>

namespace KinKal {
// templated on the data vector dimension
  template <size_t DDIM> class TData : public TDataBase {
    public:
      // define the parameter types
      typedef ROOT::Math::SVector<double,DDIM> DVec; // data vector
      typedef ROOT::Math::SMatrix<double,DDIM,DDIM,ROOT::Math::MatRepSym<double,DDIM> > DMat;  // associated matrix
      constexpr static size_t PDim() { return DDIM; }

      // construct from vector and matrix
      TData(DVec const& pars, DMat const& pcov,DataType dtype=param) : TDataBase(dtype), vec_(pars), mat_(pcov) {}
      TData(DVec const& pars) : TDataBase(param), vec_(pars) {}
      TData(DataType dtype=param) : TDataBase(dtype) {}
      // inversion changes from params <-> weight. 
      // Invert in-place, overriding status
      void invert() {
	// first invert the matrix
	if(mat_.Invert()){
	  vec_ = mat_*vec_;
	  // update base class members
	  TDataBase::invert();
	} else
	  setStatus(invalid);
      }
      // invert a different object
      void invert(TData const& other) { 
	*this = other;
	invert();
      }

      // override copy constructor, with optional inversion
      TData(TData const& other,bool doinvert=false) : TDataBase(other), vec_(other.vec()), mat_(other.mat()) {
	if(doinvert)invert();
      }

      // accessors
      DVec const& vec() const { return vec_; }
      DMat const& mat() const { return mat_; }
      DVec& vec() { return vec_; }
      DMat& mat() { return mat_; }
    private:
      DVec vec_; // parameter or weight vector
      DMat mat_; // covariance or weight matrix
  };

}
#endif

