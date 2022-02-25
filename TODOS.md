# TODOs
* addons
  * [ ] addons lib
  * [ ] addons: check/fix install
* lib
  * [ ] libco: local copy for fallback
  * [x] lib: use local .so in simulation
* threads
  * [x] startup-threads: run at end of init instead of via vpi callback
  * [x] threads: allow standalone thread creation
  * [x] examples: thread creation
  * [x] boost-impl: cleanup thread spawning workaround
  * [x] stimc++: thread creation wrapper with cleanup
* cleanup
  * [x] distinguish cleanup-reason
  * [x] simulator dependent final cleanup (cleanup callback removal, causes problems in xcelium)
* events
  * [ ] methods triggered by event (non-thread)
* doc
  * [ ] cleanup
  * [ ] standalone thread spawning
  * [ ] changelog
  * [ ] examples: build lib beforehand
* flow
  * [ ] simulation flow update (cvc, ...)
* release
  * [ ] release 1.2
