#include "KinKal/LHelix.hh"
#include "KinKal/BField.hh"
#include "Math/AxisAngle.h"
#include <math.h>
#include <stdexcept>

using namespace std;
using namespace ROOT::Math;

namespace KinKal {
  vector<string> LHelix::paramTitles_ = {
    "Transverse Radius",
    "Longiduinal Wavelength",
    "Cylinder Center X",
    "Cylinder Center Y",
    "Azimuth at Z=0 Plane",
    "Time at Z=0 Plane"}; 
  vector<string> LHelix::paramNames_ = {
  "Radius","Lambda","CenterX","CenterY","Phi0","Time0"};
  vector<string> LHelix::paramUnits_ = {
  "mm","mm","mm","mm","radians","ns"};
  std::vector<std::string> const& LHelix::paramNames() { return paramNames_; }
  std::vector<std::string> const& LHelix::paramUnits() { return paramUnits_; }
  std::vector<std::string> const& LHelix::paramTitles() { return paramTitles_; }
  std::string const& LHelix::paramName(ParamIndex index) { return paramNames_[static_cast<size_t>(index)];}
  std::string const& LHelix::paramUnit(ParamIndex index) { return paramUnits_[static_cast<size_t>(index)];}
  std::string const& LHelix::paramTitle(ParamIndex index) { return paramTitles_[static_cast<size_t>(index)];}

  LHelix::LHelix( Vec4 const& pos0, Mom4 const& mom0, int charge, double bnom, TRange const& range) : LHelix(pos0,mom0,charge,Vec3(0.0,0.0,bnom),range) {}
  LHelix::LHelix( Vec4 const& pos0, Mom4 const& mom0, int charge, Vec3 const& bnom, TRange const& trange) : KInter(mom0.M(),charge), trange_(trange), bnom_(bnom), needsrot_(false) {
    static double twopi = 2*M_PI; // FIXME
    // Transform into the system where Z is along the Bfield.
    Vec4 pos(pos0);
    Mom4 mom(mom0);
    if(fabs(bnom_.Theta()) >1.0e-6){
      needsrot_ = true;
      Rotation3D rot(AxisAngle(Vec3(sin(bnom_.Phi()),-cos(bnom_.Phi()),0.0),bnom_.Theta()));
      pos = rot(pos);
      mom = rot(mom);
      // create inverse rotation
     brot_ = rot.Inverse();
      // check
      auto test = rot(bnom_);
      if(fabs(test.Theta()) > 1.0e-6)throw std::invalid_argument("BField Error");
    }
    // compute some simple useful parameters
    double pt = mom.Pt(); 
    double phibar = mom.Phi();
    // translation factor from MeV/c to curvature radius in mm, B in Tesla; signed by the charge!!!
    double momToRad = 1000.0/(charge_*bnom_.R()*CLHEP::c_light);
    // reduced mass; note sign convention!
    mbar_ = -mass_*momToRad;
    // transverse radius of the helix
    param(rad_) = -pt*momToRad;
    // longitudinal wavelength
    param(lam_) = -mom.Z()*momToRad;
    // time at z=0
    double om = omega();
    param(t0_) = pos.T() - pos.Z()/(om*lam());
    // compute winding that puts phi0 in the range -pi,pi
    double nwind = rint((pos.Z()/lam() - phibar)/twopi);
    //  cout << "winding number = " << nwind << endl;
    // azimuth at z=0
    param(phi0_) = phibar - om*(pos.T()-t0()) + twopi*nwind;
    // circle center
    param(cx_) = pos.X() + mom.Y()*momToRad;
    param(cy_) = pos.Y() - mom.X()*momToRad;
  }

  LHelix::LHelix( PDATA const& pdata, double mass, int charge, double bnom, TRange const& range) : LHelix(pdata,mass,charge,Vec3(0.0,0.0,bnom),range) {}
  LHelix::LHelix( PDATA const& pdata, double mass, int charge, Vec3 const& bnom, TRange const& trange) : 
     KInter(mass,charge), trange_(trange), pars_(pdata), bnom_(bnom) {
      double momToRad = 1000.0/(charge_*bnom_.R()*CLHEP::c_light);
      // reduced mass; note sign convention!
      mbar_ = -mass_*momToRad;
    }
  
  double LHelix::momentumVar(float time) const {
    PDATA::DVEC dMomdP(rad(), lam(),  0.0, 0.0 ,0.0 , 0.0);
    dMomdP *= mass()/(pbar()*mbar());
    return ROOT::Math::Similarity(dMomdP,params().covariance());
  }

  void LHelix::position(Vec4& pos) const {
    Vec3 temp;
    position(pos.T(),temp);
    pos.SetXYZT(temp.X(),temp.Y(),temp.Z(),pos.T());
  }
  
  void LHelix::position(float time, Vec3& pos) const {
    // compute azimuthal angle
    double df = dphi(time);
    double phival = df + phi0();
    // now compute position
    pos.SetX(cx() + rad()*sin(phival));
    pos.SetY(cy() - rad()*cos(phival));
    pos.SetZ(df*lam());
    if(needsrot_) pos = brot_(pos);
 } 

  void LHelix::momentum(float time,Mom4& mom) const{
    double phival = phi(time);
    double factor = mass_/mbar_;
    mom.SetPx(factor * rad() * cos(phival));
    mom.SetPy(factor * rad() * sin(phival));
    mom.SetPz(factor * lam());
    mom.SetM(mass_);
    if(needsrot_) mom = brot_(mom);
  }

 void LHelix::velocity(float time,Vec3& vel) const{
    Mom4 mom;
    momentum(time,mom);
    vel = mom.Vect()*CLHEP::c_light/fabs(Q()*ebar());
    if(needsrot_)vel = brot_(vel);
  }

  void LHelix::direction(float time,Vec3& dir) const{
    Mom4 mom;
    momentum(time,mom);
    dir = mom.Vect().Unit();
    if(needsrot_)dir = brot_(dir);
  }

// derivatives of momentum projected along the given basis WRT the 6 parameters, and the physical direction associated with that
  void LHelix::momDeriv(MDir mdir, float time, PDER& pder, Vec3& unit) const {
    // compute some useful quantities
    double bval = beta();
    double omval = omega();
    double pb = pbar();
    double dt = time-t0();
    double phival = omval*dt + phi0();
    double norm = 1.0/copysign(pbar(),mbar_); // sign matters!
    // cases
    switch ( mdir ) {
      case theta1:
	// polar bending: only momentum and position are unchanged
	pder[rad_] = lam();
	pder[lam_] = -rad();
	pder[t0_] = -dt*rad()/lam();
	pder[phi0_] = -omval*dt*rad()/lam();
	pder[cx_] = -lam()*sin(phival);
	pder[cy_] = lam()*cos(phival);
	// set unit vector
	unit.SetX(lam()*cos(phival));
	unit.SetY(lam()*sin(phival));
	unit.SetZ(-rad());
	unit *= norm;
	break;
      case theta2:
	// Azimuthal bending: R, Lambda, t0 are unchanged
	pder[rad_] = 0.0;
	pder[lam_] = 0.0;
	pder[t0_] = 0.0;
	pder[phi0_] = copysign(1.0,omval)*pb/rad();
	pder[cx_] = -copysign(1.0,omval)*pb*cos(phival);
	pder[cy_] = -copysign(1.0,omval)*pb*sin(phival);
	// set unit vector
	unit.SetX(-sin(phival));
	unit.SetY(cos(phival));
	unit.SetZ(0.0);
	break;
      case momdir:
	// fractional momentum change: position and direction are unchanged
	pder[rad_] = rad();
	pder[lam_] = lam();
	pder[t0_] = dt*(1.0-bval*bval);
	pder[phi0_] = omval*dt;
	pder[cx_] = -rad()*sin(phival);
	pder[cy_] = +rad()*cos(phival);
	// set unit vector
	direction(time,unit);
	break;
      default:
	throw std::invalid_argument("Invalid direction");
    }
    if(needsrot_) unit = brot_(unit);
  }

  void LHelix::rangeInTolerance(TRange& drange, BField const& bfield, float dtol, float ptol) const {
    // compute scaling factor
    float spd = speed(drange.low());
    float sfac = spd*spd/(bnom_.R()*pbar());
    // loop over the trajectory in fixed steps to compute integrals and domains.
    // step size is defined by momentum direction tolerance or field change (last part not implemented needs gradient calc FIXME!)
    float tstep = dtol*ebar()/CLHEP::c_light;
    float dx(0.0);
    // advance till spatial distortion exceeds position tolerance or we reach the range limit
    while(fabs(dx) < ptol && drange.high() < range().high()) {
      Vec3 tpos, bvec;
      position(drange.high(),tpos);
      bfield.fieldVect(bvec,tpos);
      // BField diff with nominal
      auto dbvec = bvec - bnom_;
      // spatial distortion accumulation
      dx += sfac*drange.range()*tstep*dbvec.R();
      // increment the range
      drange.high() += tstep;
    }
  }
 
  void LHelix::print(std::ostream& ost, int detail) const {
    auto perr = params().diagonal(); 
    ost << " LHelix " << range() << " parameters: ";
    for(size_t ipar=0;ipar < LHelix::npars_;ipar++){
      ost << LHelix::paramName(static_cast<LHelix::ParamIndex>(ipar) ) << " " << paramVal(ipar) << " +- " << perr(ipar);
      if(ipar < LHelix::npars_-1) ost << " ";
    }
    if(needsrot_) ost << " with rotation around Bnom " << bnom_;
    ost << std::endl;
  }

  std::ostream& operator <<(std::ostream& ost, LHelix const& lhel) {
    lhel.print(ost,0);
    return ost;
  }

} // KinKal namespace
