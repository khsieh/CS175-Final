#include <iostream>
#include <vector>
#include <cmath>

#include "raycast.h"
#include "ppm.h"
#include "scene.h"
#include <time.h>

using namespace std;

static Scene scene;
static Camera camera;
static int samples = 256;

// camera position
static Cvec3 cameraPosition;

// this is computed from camera.fovY
static double pixelSize;

// lens parameters
static int focaldepth = 3;
static double focallength = 50;
static double aperture = 2.8;
static double coc = .029; // circle of confusion for 35mm camera based on d/1500 where d = diagonal size of camera sensor

// (x,y) are between (0,0) and (camera.width, camera.height)
static Ray computeScreenRay(const double x, const double y) {
  Cvec3 pixelPosition((x - camera.width/2) * pixelSize, (y - camera.height/2) * pixelSize, -1);
  return Ray(cameraPosition, pixelPosition);
}

// This function is basically a random-number generator (that generates samples that are well-distributed for sampling (Halton numbers))
template <int BASE>
static float haltonNumber(const int i) {
  static const float coef = 1.f / static_cast <float> (BASE);
  float inc = coef;
  float re = 0;
  unsigned int c = i;
  while (c) {
    const int d = c % BASE;
    c -= d;
    re += d * inc;
    inc *= coef;
    c /= BASE;
  }
  return re;
}

// calculates area of the camera's iris
static double calcApArea (double focallength, double aperture)
{
	return (CS175_PI * pow((focallength / (2 * aperture)), 2));
}

int main() {
  // parse the input
  parseSceneFile("scene.txt", camera, scene);

  // random seed
  srand( time(NULL) );

  // lens parameters
  int area = (int) calcApArea(focallength, aperture); // area of the camera's iris

  cameraPosition = Cvec3(0,0,0);                // we asume that the camera is at (0,0,0)
  pixelSize = 2. * sin(0.5 * camera.fovY * CS175_PI / 180.) / static_cast <double> (camera.height);

  // we precompute the multiresolution sampling pattern for a pixel
  vector<Cvec3> sampleLocation(samples);
  for (int i = 0; i < samples; ++i) {
    sampleLocation[i][0] = (haltonNumber <17> (19836 + i));   // some pseudo-random number that is well-distributed for sampling
    sampleLocation[i][1] = (haltonNumber <7> (1836 + i));     // some pseudo-random number that is well-distributed for sampling
  }

  cerr << "Rendering... ";
  vector<PackedPixel> frameBuffer(camera.width * camera.height);
  for (int y = 0; y < camera.height; ++y) {
    for (int x = 0; x < camera.width; ++x) {
      Cvec3 color(0,0,0);
      for (int i = 0; i < samples; ++i) {
		  Ray screenRay = computeScreenRay(x, camera.height - 1 - y);
		  Cvec3 focusPoint = cameraPosition + screenRay.direction * focaldepth;
		  // Cvec3 newPos = cameraPosition + Cvec3(sampleLocation[i][0], sampleLocation[i][1], 0);
		  Cvec3 newPos = cameraPosition + Cvec3((rand() % area)/500., (rand() % area)/500., 0);
		  Cvec3 newDir = (focusPoint - newPos).normalize();
		  Ray newRay(newPos, newDir);
		  color += rayTrace(scene, newRay);
      }
      color *= (1. / static_cast <double> (samples));
      PackedPixel& p = frameBuffer[x + y*camera.width];
      p.r = static_cast <unsigned char> (min(255., max(0., 255.*color[0])));
      p.g = static_cast <unsigned char> (min(255., max(0., 255.*color[1])));
      p.b = static_cast <unsigned char> (min(255., max(0., 255.*color[2])));
    }
    cerr << ".";
  }
  cerr << "done.\n";

  ppmWrite("output_image.ppm", camera.width, camera.height, frameBuffer);

  return 0;
}