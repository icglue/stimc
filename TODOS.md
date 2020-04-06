# TODOs
- provide makefile with system-wide installation?
- consider free-functionality
  - [x] some end-of-simulation // before-reset callback for modules + coroutines cleanup
  - [x] for every thread: cleanup callback linked list, user can add callbacks
  - [x] stimc++ thread cleanup convenience version -> cleanup class
  - [x] thread wrapper struct with cleanup callbacks + wait callback (to remove)
  - [x] nba-callbacks: remove with port-free function!
  - [x] events: add pointer to queue-cleanup struct -> cancel if event deleted
  - [x] for stimc: cleanup callback list; contains cleanup callbacks for
    - [x] every thread (call cleanup callbacks + delete thread)
    - [x] every method callback (remove callback)
    - [x] every module: delete module
    - [x] every port/parameter: delete
    - [x] events: clear queue (in case event is global variable)
    - [x] stimc: remove cleanup callbacks, clean event queue
  - [x] cleanup order? threads -> methods -> event queues -> modules (+ports) -> stimc-base ?
  - [ ] cleanup documentation

