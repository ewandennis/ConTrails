# An Airline Noise Heatmap

This little repo will pull flight tracks from a MongoDB and produce a heatmap of airline noise.

## Prerequisites

 - [Cairo graphics library](https://cairographics.org/): for PNG rendering - tested on v1.14.8
 - libmongoc 1.0 and libbson 1.0: for flight track data access
 - C++ compiler with C++11 support - tested on Apple LLVM version 8.0.0 (clang-800.0.42.1)
 - GNU Make 3.81

## Flight Track Data

 - Track: FlightPos[]
 - FlightPos: latitude, longitude, altitude
 - All values are double precision floating point numbers

## Assumptions
 - The listener is at sealevel: their altitude is 0
 - Aircraft are considered point sources of sound
 - Sound perception is not accounted for (ie no A-weighting is calculated)
 - Atmospheric conditions used for absorption losses:
   - frequency: 250Hz
   - temperature: 20C
   - humidity = 30%
   - ergo absorption coefficient = 1.42 (reference)[https://en.wikibooks.org/wiki/Engineering_Acoustics/Outdoor_Sound_Propagation]

## The Calculation

1. Distance field: each cell contains distance to closest flight track
1. Noise field: each cell contains dB SPL from sea level of closest flight track
1. Heatmap: 4-step colour map from normalised noise field cell values:
  - Blue, green and yellow: below noise level of a car @ 10m
  - Red: between car @ 10m and plane @ 100m

## Issues And Limitations

 - Sound perception is not accounted for (ie the map is not A-weighted)
 - Flight track data is inaccurate - some flights seem to land kilometres from GLA
 - Only the closest flight is considered, no weighting is applied for flight path probability

