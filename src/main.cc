/**
 * @file main.cc
 * @brief Application entry point: constructs the UI and runs the main loop.
 */
#include <iostream>
#include <memory>
#include <string>

#include "UI.hh"

/**
 * @brief Program entry point.
 * @return 0 on normal exit.
 */
int main() {
  UI ui = UI();
  ui.run();
  return 0;
}
