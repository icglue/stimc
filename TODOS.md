# TODOs
- provide makefile with system-wide installation?
- consider free-functionality
  - [x] some end-of-simulation // before-reset callback for modules + coroutines cleanup
  - [ ] difficult: memory allocated in threads (will be lost if threads are cancelled)
        maybe: some simplified way of adding frees/deletes as end-thread callback
  - [ ] for every thread: cleanup callback linked list, user can add callbacks
  - [ ] for stimc: cleanup callback list; contains cleanup callbacks for
    - [ ] every thread (call cleanup callbacks + delete thread)
    - [x] every method callback (remove callback)
    - [ ] nba-callbacks ??? temporary (circular?) doubly-linked-list for cleanup?
          remove with port-free function!
    - [ ] every module: delete module
    - [x] stimc: remove cleanup callbacks, clean event queue
