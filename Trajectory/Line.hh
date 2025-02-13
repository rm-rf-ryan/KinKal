#ifndef KinKal_Line_hh
#define KinKal_Line_hh

//
//  Linear time-based trajectory with a constant velocity.
//  Used as part of the kinematic Kalman fit
//
#include <memory>
#include "KinKal/General/Vectors.hh"
#include "KinKal/General/TimeRange.hh"
#include "KinKal/Trajectory/DistanceToTime.hh"
#include "KinKal/Trajectory/SensorLine.hh"

namespace KinKal {
  class Line {
    public:
      // construct from a spacetime point (typically the measurement position and time) and propagation velocity (mm/ns).
      Line(VEC4 const& p0, VEC3 const& svel, double length);
      Line(VEC3 const& p0, double t0, VEC3 const& svel, double length);
      // construct from 2 points plus timing information.  P0 is the measurement (near) end, p1 the far end.  Signals propagate from far to near
      Line(VEC3 const& p0, VEC3 const& p1, double t0, double speed );
      Line(VEC3 const& p0, double t0, VEC3 const& svel, double length, std::shared_ptr<DistanceToTime> d2t);
      // accessors
      double t0() const { return t0_; }
      double& t0() { return t0_; } // detector updates need to refine t0

      // signal ends at pos0
      VEC3 startPosition() const { return sline_.startPosition(); }
      VEC3 const& endPosition() const { return sline_.endPosition(); }
      double speed(double time) const { return d2t_->speed(d2t_->distance(time-t0_)); }
      double length() const { return sline_.length(); }
      VEC3 const& direction() const { return sline_.direction(); }
      // TOCA to a point
      double TOCA(VEC3 const& point) const;
      // geometric accessors
      VEC3 position3(double time) const;
      VEC4 position4(double time) const;
      VEC3 velocity(double time) const;
      VEC3 const& direction(double time) const { return sline_.direction(); }
      void print(std::ostream& ost, int detail) const;
      double timeAtMidpoint() const { return t0_ + d2t_->time(0.5*length()); }

    private:
      double t0_; // intial time (at pos0)
      std::shared_ptr<DistanceToTime> d2t_; // represents the possibly nonlinear distance to time relationship of the line
      SensorLine sline_; // geometic representation of the line
  };
  std::ostream& operator <<(std::ostream& ost, Line const& tline);
}
#endif
