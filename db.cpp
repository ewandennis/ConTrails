#include <vector>
using std::vector;

#include <algorithm>

#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

#include "db.h"

static const bool s_useTestData = false;

static bool readNumericFld(bson_iter_t& _it, const char* _fld, double &r_val) {
  if (!bson_iter_find(&_it, _fld))  {
    return false;
  }
  if (BSON_ITER_HOLDS_DOUBLE(&_it)) {
    r_val = bson_iter_double(&_it);
    return true;
  } else if (BSON_ITER_HOLDS_INT32(&_it)) {
    r_val = double(bson_iter_int32(&_it));
    return true;
  }
  return false;
}

bool pullFlightTracksFromMongo(const char *_mongouri, const char* _db,
    const char* _coll, TrackVec& r_tracks) {
  static double maxAlt = 0;
  static double minAlt = 10000000;
  if (s_useTestData) {
    r_tracks.push_back(PosVec());
    r_tracks.back().push_back(FlightPos(-5.51, 55.5, 50));
    r_tracks.back().push_back(FlightPos(-5.50, 55.51, 1));
    return true;
  }
  mongoc_init ();
  mongoc_client_t *client = mongoc_client_new (_mongouri);
  mongoc_client_set_appname (client, "contrails");
  mongoc_database_t *database = mongoc_client_get_database (client, _db);
  mongoc_collection_t *collection = mongoc_client_get_collection (client, _db, _coll);

  bson_t query;
  bson_init(&query);
  mongoc_cursor_t *cursor;
  cursor = mongoc_collection_find_with_opts(collection, &query, NULL, NULL);

  const bson_t *doc;
  int numMartianSamples = 0;
  while(mongoc_cursor_next(cursor, &doc)) {
    bson_iter_t docit;
    bson_iter_t trackit;
    if (!bson_iter_init_find(&docit, doc, "track") ||
        !BSON_ITER_HOLDS_ARRAY(&docit)  ||
        !bson_iter_recurse(&docit, &trackit)) {
      continue;
    }

    r_tracks.push_back(PosVec());
    PosVec &track = r_tracks.back();
    while(bson_iter_next(&trackit)) {
      FlightPos pos;
      int fldCount = 0;
      bson_iter_t sampleit;
      if (!bson_iter_recurse(&trackit, &sampleit)) {
        continue;
      }

      if (readNumericFld(sampleit, "latitude", pos.lat)) {
        fldCount |= 0x1;
      }

      if (readNumericFld(sampleit, "longitude", pos.lng)) {
        fldCount |= 0x2;
      }

      if (readNumericFld(sampleit, "altitude", pos.alt)) {
        fldCount |= 0x4;
        maxAlt = std::max(pos.alt, maxAlt);
        minAlt = std::min(pos.alt, minAlt);
      }

      if (fldCount == 0x7) {
        track.push_back(pos);
      } else {
        ++numMartianSamples;
      }
    }
  }

  bson_destroy(&query);
  mongoc_cursor_destroy(cursor);
  mongoc_collection_destroy (collection);
  mongoc_database_destroy (database);
  mongoc_client_destroy (client);
  mongoc_cleanup ();

  return true;
}

