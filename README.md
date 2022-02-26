# stimc – a lightweight Verilog-vpi Wrapper for Stimuli Generation

stimc is a lightweight Verilog-vpi wrapper to simplify simulation control via
c/c++ code similar to SystemC.
In contrast to SystemC you can only use stimc together with a Verilog simulator
and it is not meant as a standalone hardware description or modelling language.
The main Purpose is to control and observe ports of an empty Verilog shell-module
via c/c++ code to provide abstract models for external components or emulate functionality
of an external software-based access to a hardware component.

## Usage

### Verilog Shell
The first thing you need is a Verilog shell for a stimc controlled module.
This is a Verilog module with parameters and inputs/outputs and optionally parameters.
The module should have no actual content except for an initial block calling the system task
`$stimc_<modulename>_init();` (with `<modulename>` replaced by the name of the Verilog
module name.
The system task will be defined by stimc and initialize the stimc part of the module.

An example could look like this:
```verilog
module dummy (
    clk_i,
    reset_n_i,
    data_in_i,
    data_out_o
);

    parameter DATA_W = 32;

    input               clk_i;
    input               reset_n_i;
    input  [DATA_W-1:0] data_in_i;
    output [DATA_W-1:0] data_out_o;

    initial begin
        $stimc_dummy_init();
    end

endmodule
```

### stimc Module
stimc core is written in c with a thin c++ wrapper,
so the stimc part of the module can be written in c or c++,
but c++ will produce more readable code and here the c++ version will be explained.
The c++ code is wrapped in a `stimcxx` namespace.

#### Module Class
First a c++ class is needed that inherits `stimcxx::module` (include `stimc++.h`).
For every (used) port of the Verilog shell the class should have a `port` member,
for every (used) parameter a `parameter` member of the same name as the Verilog pendant.
Additionally, it can have `stimcxx::event` members in case waiting on events triggered
by individual stimc threads or value-change events on ports is needed (similar to
SystemC events).

The class needs a constructor without arguments and can have (non-static) void functions
that can be run as initially started threads (comparable to a Verilog initial block
or a SystemC thread) or port event based in which case they are limited to triggering
events or immediately reacting (similar to SystemC methods).

An excerpt from the project's examples looks like this:
```cpp
class dummy : public stimcxx::module {
    private:
        parameter DATA_W;

        port clk_i;
        port reset_n_i;
        port data_in_i;
        port data_out_o;

        stimcxx::event clk_event;

    public:
        dummy ();
        virtual ~dummy ();

        void testcontrol ();
        void clock ();
        void dinchange ();
};
```

#### Constructor
The constructor needs to initialize ports, parameters, event-triggered methods and threads
started at simulation start.
For ports and parameters preprocessor defines are prepared to use in the member initializer list.
For ports this is `STIMCXX_PORT (<port-name>)`, for parameters `STIMCXX_PARAMETER (<parameter-name>)`.
In the constructor threads and event-triggered methods can be setup with the defines
`STIMCXX_REGISTER_STARTUP_THREAD (<function-name>);` and
`STIMCXX_REGISTER_METHOD (<event-type>, <port-name>, <function-name>);`, where `<event-type>` is
one of `posedge`, `negedge` or `change` (corresponding to the Verilog posedge, negedge or just
plain `@` events).

The main part of the example constructor looks like this:
```cpp
dummy::dummy () :
    STIMCXX_PARAMETER (DATA_W),
    STIMCXX_PORT (clk_i),
    STIMCXX_PORT (reset_n_i),
    STIMCXX_PORT (data_in_i),
    STIMCXX_PORT (data_out_o)
{
    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (change, data_in_i, dinchange);
}
```

On every posedge event on the `clk_i` input the member function `clock` will
be called, on every value change on input `data_in_i` the function `dinchange` will
be called and a stimc thread will be created and run at simulation start calling
`testcontrol`.

#### Initialization
To provide the Verilog initializer system task and functionality
the preprocessor define `STIMCXX_EXPORT (<module-name>)` (without semicolon)
should be put at some place in the code.

This will provide the system tasks for all modules on loading the vpi library.

#### Functionality
To interact with simulation, parameters can be read, ports can be read and assigned.
For reading it is possible to directly assign a port or parameter to a variable.
For writing to a port you can assign an integer type to it directly (corresponding
to Verilog blocking assignment) or via the overloaded `<<=` operator (corresponding
to Verilog non-blocking assignment, optical similarity to Verilog is intended,
while risking misinterpretation as shift operation).

In case only a bit-range of a port should be accessed or an integer type is not sufficiently
wide it is possible to access the ports `operator() (<msb>,<lsb>)` and similarly read from
or write to the bit-range.
Ports can also be assigned to verilog `x` or `z` values using the similarly named constants `X`
and `Z` or checked to contain unknown values via comparison against `X`.

### Threads and Events
Apart from a module method it is also possible to spawn a standalone function taking
a single pointer argument as a thread via `STIMCXX_SPAWN_THREAD (<function>, <data>);`
Threads can be suspended to wait for a specified amount of simulation time and/or an
event to be triggered.

There are some helper functions in the `stimcxx` namespace:
* Multiple wait functions.
  They can only be called from within a thread
  (as only initialized threads can be suspended and resumed):
  * `wait(<event>)` to wait on an event to be triggered,
  * `wait(<event1> & <event2> & ...)` to wait for all of the events to be triggered,
  * `wait(<event1> | <event2> | ...)` to wait for any of the events to be triggered,
  * `wait(<time>)` to wait an amount of time in seconds (double) or
  * `wait(<time>, <unit>)` to wait an integral amount of time in specified unit (similarly `SC_PS` and so on).
  * `wait(<event or combination>, <time>)`, `wait(<event or combination>, <time>, <unit>)` to wait for
    an event or combination (any/all) of events with the specified time as timeout.
    In contrast to the other wait functions the wait with timeout returns `true` in case of timeout,
    `false` otherwise.
* `time()` (current simulation time in seconds), `time(<unit>)` (current simulation
  time as integer in specified unit, similar to SystemC units are defined as `SC_PS`, `SC_US` and so on).
* `finish()` to finish simulation.

Events can be triggered by calling their `trigger()` member function.
When triggering an event, all threads currently waiting for it will be resumed (in any order).
It is not possible for a thread to trigger itself as it would either need to first wait (not being
able to trigger) or first trigger (and not yet waiting on the event, so not being resumed on this
trigger).

### Cleanup
Resource cleanup is mainly useful in cases where simulation can be reset.
Threads will be recreated and run after a reset and might cause conflicts
with remaining resources from a previous run.

While modules are cleaned up by their destructor being called at end of simulation,
threads could be suspended in a waiting state, having their resources still acquired.

#### Stack unwinding
Since version 1.2 a stimc++ thread's stack will by default be unwound on termination at and
of simulation or simulator reset. If programmed accordingly by writing the associated thread
code exception safe it is possible to have resources freed this way.

Stack unwinding is achieved by letting `wait` functions throw a stimc++ specific exception
at end of simulation which therefore should not be caught or otherwise rethrown if necessary.
It is important to consider, that all thread's stacks can be unwound in any order.

If stack unwinding causes problems it can be disabled by defining `STIMCXX_DISABLE_STACK_UNWIND`.
Alternatively it can be selectively disabled for individual threads by calling
`stimc_thread_resume_on_finish (false)`.

#### Explicit cleanup
In order to explicitly control freeing of memory or cleanup of resources like open files
when a thread is terminated it is possible to register a cleanup callback.

In stimc++ it is possible to wrap this into one or more objects of custom cleanup classes
inheriting `stimcxx::thread_cleanup` which is created within the thread via `new` and performs
cleanup tasks in its destructor.
The creation via new is important, as the object must also remain in case the thread function returns
and will be destroyed via `delete` in the cleanup process.
The object must be created within the thread it should cleanup as it will be called when the respective
thread is deleted.
Furthermore it is necessary to cover all possible scenarios where the cleanup could happen, which
is typically whenever the thread is destroyed while suspended (waiting, which might also happen in
a function calling a wait function) or when the thread function returns.
In all of these cases the cleanup in the destructor should do the right thing (e.g. free
allocated resources and ignore not yet allocated or already freed resources).

Here an example with a snippet from a thread and a cleanup class for 2 variables a and b
being objects of two classes A and B shows the general idea:

```cpp
/* custom cleanup class */
class cleanup_ab : protected stimcxx::thread_cleanup {
    public:
        A *a;
        B *b;

        cleanup_ab () : a(nullptr), b(nullptr) {}
        ~cleanup_ab () {
            if (a) delete a;
            if (b) delete b;
        }
};

/* simulation thread of custom module dummy */
void dummy::testcontrol ()
{
    cleanup_ab *cleanup_data = new cleanup_ab ();

    /* ... */

    A *a = new A ();
    /* from here on, if thread is deleted, a will be deleted */
    cleanup_data->a = a;

    /* ... */

    B *b = new B ();
    /* from here on, if thread is deleted, a and b will be deleted */
    cleanup_data->b = b;

    /* ... */

    delete a;
    /* a already freed -> do not delete when thread is deleted */
    cleanup_data->a = nullptr;

    /* ... */
}

```

In case cleanup functionality causes unwanted problems (use after free or similar),
it is possible to disable it via the `STIMC_DISABLE_CLEANUP` preprocessor define
when compiling stimc.
In this case besides memory being leaked, resetting simulation will likely cause problems.

## Building
### Compiling
For compiling everything (depending on the simulator) you need to build a vpi library
containing the stimc/stimc++ code or linking against stimc as a shared library, and
the module export code created with the `STIMCXX_EXPORT (<module-name>)` macro.

The vpi library, if compiled with stimc, or the linked stimc library needs to be linked against
the used coroutine library for thread implementation, by default `pcl`.
Alternatively coroutines can be implemented with libco (preprocessor define `STIMC_THREAD_IMPL_LIBCO`
and link against `libco`) or boost coroutines (preprocessor define
`STIMC_THREAD_IMPL_BOOST2` and link against `boost_context` for `boost/coroutine2` implementation
or define `STIMC_THREAD_IMPL_BOOST1` and link against `boost_coroutine` for `boost/coroutine`
implementation). Although boost would probably not count as lightweight, it is probably
one of the more available alternatives.

vpi headers are by default included via icarus verilog. In case a different simulator
is to be used, the relevant include path might be changed.

### Installation
There is no need to install stimc, as you can just compile the sources together with your
own code into a vpi shared library for the simulator to load.
But it is also possible to compile it into a standalone shared library to link against the
vpi library. This can be achieved via running
```shell
make
make install
```
and can be controlled by
* `THREAD_IMPL` (one of `pcl`, `libco`, `boost1` or `boost2`, depending on available
  coroutine implementation libraries),
* `PREFIX` (install prefix, e.g. `/usr`),
* `DESTDIR` (e.g. your temporary package directory)
variables.

## Documentation
### Documentation
Doxygen documentation for the code can be built by running `make docs` when doxygen is available.
To open generated docs run `make showdocs` (will open html documentation in firefox, which
can be overwritten with `make BROWSER=<browser> showdocs`.

### Examples
Examples are provided in the examples directory.
To use it you need icarus verilog, GTKWave and the portable coroutine library (libpcl) installed.
You can load the project environment by sourcing the `env.sh` in the project examples directory.
Simulation of the given examples depends on stimc being precompiled as a library
(run `make` in the project's root directory first).

The examples provide 3 stimc design units: dummy (the shown dummy, also in a plain c version
for reference), ff (a stimc flip-flop model)
and ff\_real (flip-flop with real-value port interface).
The Verilog shell can be found in `source/behavioral/verilog` of the given unit, the module stimc code in
`source/behavioral/stimc`. To run example simulations enter the unit's `simulation/iverilog/tc_<...>`
testcase directory and run `make`. The code will be compiled, run and GTKWave will be started
for browsing the simulated waveforms.

## License
stimc itself is licensed under the GNU LGPLv3. But depending on the used coroutine library the
derived work might fall under the coroutine library's license. `pcl` is licensed
under GNU GPL, so here the combination is also licensed as GPL, whereas when using `libco` or `boost`
the combination will be licensed under GNU LGPL.
