////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Text overlay
//

#include <AntTweakBar\AntTweakBar.h>
#define ANT_TWEAK_BAR
#include "../../octet.h"

#include "LSystems.h"


/// Create a box with octet
int main(int argc, char **argv) {
  // set up the platform.
  octet::app::init_all(argc, argv);

  // our application.
  octet::LSystems app(argc, argv);
  app.init();

  // open windows
  octet::app::run_all_apps();
}


