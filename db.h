#pragma once

#include <vector>

#include "pos.h"

bool pullFlightTracksFromMongo(const char *_mongouri, const char* _db,
    const char* _coll, TrackVec& r_tracks);
