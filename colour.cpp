#include <cmath>

// Adapted from http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients
void getHeatMapColor(float value,
    float &r_red, float &r_green, float &r_blue) {
  // A static array of 4 colors:  (blue, cyan, green, yellow, red) using {r,g,b} for each.
  const int NUM_COLORS = 5;
  static const float color[NUM_COLORS][3] = {
    {0,0,1},
    {0,1,1},
    {0,1,0},
    {1,1,0},
    {1,0,0}
  };

  int idx1;        // |-- Our desired color will be between these two indexes in "color".
  int idx2;        // |
  float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

  if (value <= 0) {
   idx1 = idx2 = 0; // accounts for an input <=0
  } else if (value >= 1) {
    idx1 = idx2 = NUM_COLORS-1; // accounts for an input >=0
  } else {
    value = value * (NUM_COLORS-1);        // Will multiply value by 3.
    idx1  = floor(value);                  // Our desired color will be after this index.
    idx2  = idx1+1;                        // ... and before this index (inclusive).
    fractBetween = value - float(idx1);    // Distance between the two indexes (0-1).
  }

  r_red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
  r_green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
  r_blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

