#include <limits>

#include <cmath>
using std::abs;

#include <list>
using std::list;

#include <iostream>
using std::cout;
using std::endl;

#include "flighttracks.h"

#include "db.h"

bool FlightTracks::fromDB(const char *_mongouri, const char* _db,
    const char* _coll, FlightTracks& r_tracks) {
  return pullFlightTracksFromMongo(_mongouri, _db, _coll, r_tracks.tracks_);
}

void FlightTracks::altitudesToMetres() {
  for(auto&& track : tracks_) {
    for(auto&& pos : track) {
      pos.altToMetres();
    }
  }
}

void FlightTracks::removeTracksUnrelatedTo(const FlightPos& _pt, double _eps) {
  list<std::size_t> unrelatedTracks;

  for(std::size_t trackIdx = 0; trackIdx < tracks_.size(); trackIdx++) {
    PosVec& track = tracks_[trackIdx];
    const FlightPos& first = track.front();
    const FlightPos& last = track.back();
    cout << last.lat << ", " << last.lng << " -> " << last.sphereDistTo(_pt) << endl;
    if (first.sphereDistTo(_pt) > _eps ||
        last.sphereDistTo(_pt) > _eps) {
      unrelatedTracks.push_back(trackIdx);
    }
  }

  while (!unrelatedTracks.empty()) {
    tracks_.erase(tracks_.begin() + unrelatedTracks.back());
    unrelatedTracks.pop_back();
  }
}

static double sqr(double _d) { return _d * _d; }

static void closestPointOnSegment(const FlightPos& p,
    const FlightPos& v, const FlightPos& w, FlightPos& r_closestPt) {

  double l2 = sqr(v.lng - w.lng) + sqr(v.lat - w.lat);

  if (l2 <= 0) {
    r_closestPt = v;
    return;
  }

  double t = ((p.lng - v.lng) * (w.lng - v.lng) + (p.lat - v.lat) * (w.lat - v.lat)) / l2;

  if (t < 0) {
    r_closestPt = v;
  } else if (t > 1) {
    r_closestPt = w;
  } else {
    r_closestPt.lng = v.lng + t * (w.lng - v.lng);
    r_closestPt.lat = v.lat + t * (w.lat - v.lat);
    r_closestPt.alt = v.alt + t * (w.alt - v.alt);
  }
}

double FlightTracks::distanceToTrack(const FlightPos& _pos, int _maxTracks) const {
  double minDist = std::numeric_limits<double>::max();
  int numTracks = 0;
  FlightPos closest;
  for(auto&& track : tracks_) {
    for(int pidx = 0; pidx < track.size()-1; pidx++) {
      closestPointOnSegment(_pos, track[pidx], track[pidx+1], closest);
      double dist = _pos.distTo(closest);
      if (dist < minDist) {
        minDist = dist;
      }
    }
    ++numTracks;
    if (numTracks >= _maxTracks) {
      break;
    }
  }
  return minDist;
}
