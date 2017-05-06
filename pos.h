#pragma once

#include <vector>

#include <cmath>

static const double s_feetPerMetre = 3.28084; // feet per metre

struct FlightPos {
  static double deg2rad(double _d) {
    return _d * M_PI / 180.0; 
  }

  double lat; // in degrees
  double lng; // in degrees
  double alt; // in hundreds of ft

  FlightPos(): lat(0), lng(0), alt(0) {}

  FlightPos(double _lng, double _lat):
    lat(_lat),
    lng(_lng),
    alt(0)
  {}

  FlightPos(double _lng, double _lat, double _alt):
    lat(_lat),
    lng(_lng),
    alt(_alt)
  {}

  // Converts this position's altitude from hundreds of ft to metres
  void altToMetres() {
    alt = (alt * 100) / s_feetPerMetre;
  }

  // Returns: spherical 2D distance between positions in metres
  // http://www.movable-type.co.uk/scripts/latlong.html
  // Note: lat and long only - altitude not accounted for.
  double sphereDistTo(const FlightPos& _rhs) const {
    double R = 6371e3; // metres
    double lat1 = deg2rad(lat);
    double lat2 = deg2rad(_rhs.lat);
    double dlat = lat2-lat1;
    double dlng = deg2rad(_rhs.lng-lng);
    double a = sin(dlat/2) * sin(dlat/2) +
            cos(lat1) * cos(lat2) *
            sin(dlng/2) * sin(dlng/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
  }

  // Returns: flat 2D distance in degrees
  // Note: lat and long only - altitude not accounted for
  double vecDistTo(const FlightPos& _rhs) {
    double dlat = _rhs.lat - lat;
    double dlng = _rhs.lng - lng;
    return std::sqrt(dlat*dlat + dlng*dlng);
  }

  // Returns: 3D distance in metres
  double distTo(const FlightPos& _rhs) const {
    double dlatlng = sphereDistTo(_rhs);
    double dalt = _rhs.alt - alt;
    return std::sqrt(dlatlng*dlatlng + dalt*dalt);
  }
};

typedef std::vector<FlightPos> PosVec;
typedef PosVec::iterator PosVecIt;

typedef std::vector<PosVec> TrackVec;
typedef TrackVec::iterator TrackVecIt;
