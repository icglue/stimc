# stimc Changelog

## Version 1.3
* Example simulation flow rework to support more simulators.
* Simulator dependent cleanup callbacks.
* Added local copy of libco as default thread implementation.
* Build rework using cmake.
* Add "patch" version based on commits since last release.
* Configurable build (coroutine lib, simulator, internal parameters).
* Internal fixes, cleanup and rework.

## Version 1.2
* Events are movable.
* stimc++ stack unwinding for thread cleanup.
* Standalone thread spawning.
* Simulator dependent cleanup callbacks.
* Internal fixes, cleanup and rework.

## Version 1.1
* VPI logging helper
* SystemC compatibility helper improved
* License changed to LGPL (depending on coroutine implementation)

## Version 1.0
* Initial release.
* Main functionality with:
  * Module port and parameter access,
  * threads via coroutines,
  * events and event combination,
  * waiting for simulation time and events.
* Examples + Test.
* Post-simulation cleanup.
