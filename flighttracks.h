#pragma once

#include "pos.h"

class FlightTracks {
public:
  static bool fromDB(const char *_mongouri, const char* _db,
    const char* _coll, FlightTracks& r_tracks);

  void altitudesToMetres();

  void removeTracksUnrelatedTo(const FlightPos& _pos, double _eps);

  size_t numTracks() const { return tracks_.size(); }

  double distanceToTrack(const FlightPos& _pos, int _maxTracks = 1000) const;
//private:
  TrackVec tracks_;
};

