/*
 * Generate a heatmap of airline noise over a given region.
 */
#include <iostream>
using std::cout;
using std::endl;
using std::flush;

#include <cmath>
using std::abs;

#include <algorithm>
using std::max;

#include <chrono>
using namespace std::chrono;

#include <cairo.h>
#include <cairo-svg.h>

#include "flighttracks.h"
#include "colour.h"

static const bool s_drawTestStroke = false;
static const bool s_drawDistanceField = false;
static const bool s_drawSoundLevelField = true;
static const bool s_drawFlightTracks = false;

static const int s_width = 1000;
static const int s_height = 1000;

static const int maxTracksForDistanceCalc = 10000;

static const bool s_pruneFlightTracks = false;
static const double maxTrackEndPtDistFromAirport = 10000;

static const char* s_filename = "layer.png";

// Campsies only
/*
static const double s_north = 55.991257858830004;
static const double s_east = -4.036719482421859;
static const double s_south = 55.87327635179371;
static const double s_west = -4.380048583984376;
*/

// Clyde valley, Glasgow, Campsies
static const double s_north = 56.01199122056832;
static const double s_east = -3.841708984374918; //-4.289408203125049; //-4.065558593749984;
static const double s_south = 55.763718140544604;
static const double s_west = -4.73710742187518; //-4.513257812500115;

// West central Scotland
/*
static const double s_north = 56;
static const double s_south = 53;
static const double s_east = -3;
static const double s_west = -6;
*/

// Tiny test region
/*
static const double s_north = 55.52;
static const double s_south = 55.49;
static const double s_east = -5.49;
static const double s_west = -5.52;
*/

// Spreading losses: -6dB / 100m
// Attenuation losses: a . r, where:
//   - a is attenuation coefficient for frequency F, temperature T, humidity H
//   - r is path length of travelling wave in metres
// Aircraft noise is complex. For this visualisation, let:
//   - F = 250Hz
//   - T = 20C
//   - H = 30%
//   - ergo a = 1.42 (https://en.wikibooks.org/wiki/Engineering_Acoustics/Outdoor_Sound_Propagation)

// From http://www.easa.europa.eu/system/files/dfu/MAdB%20JETS%20(170210).xlsx
// Loudest level flyover
static const double s_airlineNoiseReferenceLeveldB = 100;
static const double s_spreadingCoefficient = 6;
static const double s_attenuationCoefficientKM = 1.42;

// ----------------------------------------------------------------------------

static const char* s_mongouri = "mongodb://localhost:27017";
static const char* s_db = "contrails";
static const char* s_coll = "tracks";

// ----------------------------------------------------------------------------

static const double s_lngSize = abs(s_west - s_east);
static const double s_latSize = abs(s_north - s_south);

static const FlightPos GLA = {
  55.8690774,
  -4.4372416,
  0
};

static void onePixelLineWidth(cairo_t* _cxt) {
  double ux=1, uy=1;
  cairo_device_to_user_distance (_cxt, &ux, &uy);
  if (ux < uy) {
      ux = uy;
  }
  cairo_set_line_width (_cxt, ux);
}

static void drawTracks(cairo_t* _cxt, const FlightTracks& _tracks) {
  // Normalise user coordinate space: 0-1, 0-1
  cairo_identity_matrix(_cxt);
  cairo_scale(_cxt, double(s_width), double(s_height));
  for (auto&& track : _tracks.tracks_) {
    cairo_set_source_rgba(_cxt, 1, 0, 0, 0.3);
    for (auto&& sample : track) {
      double x = ((sample.lng - s_west) / s_lngSize);
      double y = ((sample.lat - s_south) / s_latSize);
      cairo_line_to(_cxt, x, y);
    }
    onePixelLineWidth(_cxt);
    cairo_stroke(_cxt);
  }
}

double* calcDistField(const FlightTracks& _tracks, double& r_maxDist) {
  double *distField = new double[s_width * s_height];
  r_maxDist = 0;
  for(int y = 0; y < s_height; ++y) {
    FlightPos pos;
    double ynorm = double(y) / double(s_height);
    pos.lat = s_south + (ynorm * s_latSize);
    for(int x = 0; x < s_width; ++x) {
      double xnorm = double(x) / double(s_width);
      pos.lng = s_west + (xnorm * s_lngSize);
      int idx = y * s_width + x;
      double dist = _tracks.distanceToTrack(pos, maxTracksForDistanceCalc);
      // Assumption: listener at sealevel: pos.alt is implicitly 0
      distField[idx] = dist;
      if (r_maxDist < dist) {
        r_maxDist = dist;
      }
    }
    cout << y << endl << flush; 
  }

  cout << "Max distance to flight path: " << r_maxDist << "m" << endl;
  return distField;
}

static double* calcSoundField(double* _distField, double _refLeveldB, double& r_maxSoundLevel) {
  double *soundField = new double[s_width * s_height];
  r_maxSoundLevel = 0;
  for(int y = 0; y < s_height; ++y) {
    for(int x = 0; x < s_width; ++x) {
      int idx = y * s_width + x;
      double dist = _distField[idx];
      double soundLevel = _refLeveldB - (dist / 100 * s_spreadingCoefficient);
      soundLevel -= dist / 1000 * s_attenuationCoefficientKM;
      soundLevel = max(soundLevel, 0.);
      soundField[idx] = soundLevel;
      if (r_maxSoundLevel < soundLevel) {
        r_maxSoundLevel = soundLevel;
      }
    }
  }
  cout << "Max sound level: " << r_maxSoundLevel << "dB" << endl;
  return soundField;
}

static void drawIntensityField(cairo_t* _cxt, double* _field, double _maxVal) {
  cairo_surface_t* img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, s_width, s_height);
  unsigned char* imgBits = cairo_image_surface_get_data(img);
  unsigned int stride = cairo_image_surface_get_stride(img);
  for(int y = 0; y < s_height; ++y) {
    for(int x = 0; x < s_width; ++x) {
      int imgOffs = (y * stride) + (x * 4);
      int fieldIdx =  y * s_width + x;
      float t = _field[fieldIdx] / _maxVal;
      float r, g, b;

      getHeatMapColor(t, r, g, b);

      // Red with transparency as intensity
      imgBits[imgOffs]   = (unsigned char)(b * t * 255.0);
      imgBits[imgOffs+1] = (unsigned char)(g * t * 255.0);
      imgBits[imgOffs+2] = (unsigned char)(r * t * 255.0);
      imgBits[imgOffs+3] = (unsigned char)(t * 255.0); // alpha
    }
  }
  cairo_surface_mark_dirty(img);
  cairo_identity_matrix(_cxt);
  cairo_scale(_cxt, double(s_width), double(s_height));
  cairo_scale(_cxt, 1.0 / cairo_image_surface_get_width(img), 1.0 / cairo_image_surface_get_height(img));
  cairo_translate(_cxt, 0, s_height);
  cairo_scale(_cxt, 1, -1);
  cairo_set_source_surface(_cxt, img, 0, 0);
  cairo_paint(_cxt);

  cairo_surface_finish(img);
  cairo_surface_destroy(img);
}

int main (int argc, char *argv[]) {
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  FlightTracks tracks;
  if (!FlightTracks::fromDB(s_mongouri, s_db, s_coll, tracks)) {
      return -1;
  }

  if (s_pruneFlightTracks) {
    cout << "Total track count: " << tracks.numTracks() << endl;
    tracks.removeTracksUnrelatedTo(FlightPos(55.8690774, -4.4372416), maxTrackEndPtDistFromAirport);
    cout << "After cleanup: " << tracks.numTracks() << endl;
  }

  // Flight position altitudes are expressed in hundreds of ft.
  // Convert to m.
  tracks.altitudesToMetres();

  // Create surface
  //cairo_surface_t *surface = cairo_svg_surface_create(s_filename, s_width, s_height);
  cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, s_width, s_height);
  cairo_t* cxt = cairo_create (surface);

  if (s_drawTestStroke) {
    cairo_identity_matrix(cxt);
    cairo_scale(cxt, double(s_width), double(s_height));
    cairo_move_to(cxt, 0.2, 0.2);
    cairo_line_to(cxt, 0.8, 0.8);
    onePixelLineWidth(cxt);
    cairo_stroke(cxt);
  }

  double maxDist = 0;
  double *distField = calcDistField(tracks, maxDist);

  if (s_drawDistanceField) {
    drawIntensityField(cxt, distField, maxDist);
  }

  double maxSoundLevel = 0;
  double *soundField = calcSoundField(distField, s_airlineNoiseReferenceLeveldB, maxSoundLevel);
  if (s_drawSoundLevelField) {
    drawIntensityField(cxt, soundField, maxSoundLevel);
  }

  if (s_drawFlightTracks) {
    drawTracks(cxt, tracks);
  }

  // Render SPL function to surface

  cairo_surface_write_to_png(surface, s_filename);
  cairo_destroy(cxt);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);

  delete [] soundField;
  soundField = 0;
  delete [] distField;
  distField = 0;

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> dt = duration_cast<duration<double>>(t2 - t1);
  cout << "Runtime: " << dt.count() << " seconds" << endl;

  return 0;
}

