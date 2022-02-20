#ifndef KinKal_MatEnv_ELossDistributions_HH
#define KinKal_MatEnv_ELossDistributions_HH

#include <iostream>
#include <vector>
#include <cmath>
#include "KinKal/MatEnv/DetMaterial.hh"

namespace KinKal {
  class MoyalDist{
    public:
      struct ModeSigma{
        ModeSigma(double mpv, double xi) : mpv_(mpv), xi_(xi) {}
        double mpv_, xi_;
      };
      struct MeanRMS{
        MeanRMS(double mean, double rms) : Mean_(mean), RMS_(rms) {}
        double Mean_; double RMS_;
      };

      MoyalDist(ModeSigma const& modesigma, int max = 20):_mode(modesigma.mpv_), _sigma(modesigma.xi_),_kmax(max) {
        _mean = _mode + _sigma *  MFACTOR;
        _rms = 0.5 * (std::pow( M_PI * _sigma , 2) );
        setCoeffs(_kmax);
      }

      MoyalDist(MeanRMS const& meanrms, int max = 20):_mean(meanrms.Mean_),_rms(meanrms.RMS_),_kmax(max) {
        //Variance of moyal = (pi * sigma)^2 /2
        _sigma = std::sqrt(2.0) * _rms / M_PI ;
        _mode = _mean - _sigma * MFACTOR;
        setCoeffs(_kmax);
      }

      double sampleAR() const;
      double sample(double rand) const; // input is a random number [0,1]
      double getMean() const { return _mean; }
      double getSigma()const { return _sigma; }
      double getRMS() const { return _rms; }

    private:
      double _mode;  //For unimodal distribution most probable value and mode are the same thing
      double _mean;  // mean of the distribution
      double _sigma; // sigma of the distribution
      double _rms;   // rms of the distribution
      int _kmax;     // Number of terms to keep in the series expansion of the inverse of cumulative distribution function
      std::vector<double> _coeff;  // Stores the coeffs needed to calculate InvCDF for a certain _kmax
      void setCoeffs(int kmax);  // Sets the values of coeffs vector.
      static constexpr double EG = 0.577215664901532860606;    //Euler-gamma constant; replace this with numerics when we move to c++20
      static constexpr double MFACTOR = EG + M_LN2;
  };

  /// Putting in a class for Bremsstrahlung loss
  class BremssLoss
  {
    public:
      double sampleSTDGamma(double energy, double radthickenss) const; //standard c++ library for gamma distribution
      double sampleSSPGamma(double energy, double radthickenss) const; //implementation of gamma distribution for small shape parameter
  };
  
  class DeltaRayLoss{
    // This class is based on the calulcations presented in the following thesis
    // Watts Jr, J. W. Calculation of energy deposition distributions for simple geometries. No. M452. 1973.
    public:
      DeltaRayLoss(const MatEnv::DetMaterial* dmat, double mom, double pathlen, double mass):
      _cutOffEnergy(dmat->eexc()),
      _beta(dmat->particleBeta(mom,mass)),
      _gamma(dmat->particleGamma(mom,mass)),
      _mass(mass)
      {
        _beta2 = _beta * _beta;
        double mratio = e_mass_/mass;
        
        _elossMax = 2*e_mass_*pow(_beta * _gamma,2)/
          (1+2*_gamma*mratio + pow(mratio,2));
        if(mass <= e_mass_){
          _elossMax *= 0.5;
        }

        _xi = (0.307/2.) * (dmat->density() * dmat->zeff()/dmat->aeff()) * (1./_beta2); // 0.307 MeV mol^-1 cm  is from PDG
        
        //The expression for the term below is true for spin 1/2 particles
        // This should be changed for spin 0 ans spin 1 but I don't think it is needed for Track Toy
        // see E.A. Uehling, Ann. Rev. Nucl. Sci. 4, 315 (1954)
        _avgNumber = _xi * pathlen * ( (1./_cutOffEnergy) - (1./_elossMax) \
                             + (_beta2/_elossMax) * std::log(_cutOffEnergy/_elossMax) \
                             + (_elossMax - _cutOffEnergy)/(_gamma * _mass));
       
                                 
      }

      double sampleDRL() const;
      void setCutOffEnergy(double cutoff) {
          _cutOffEnergy = cutoff;
      };
      double getCutOffEnergy(){
        return _cutOffEnergy;
      };

    private:
      double _cutOffEnergy; //cut off energy 
      double _elossMax; 
      double _xi; 
      double _beta;
      double _beta2;
      double _gamma;
      double _mass;
      double _avgNumber; //average number of delta rays produced along pathlen X abovecutoffEnergy
      static constexpr double e_mass_ = 5.10998910E-01; // electron mass in MeVC^2
  };
  
}
#endif
