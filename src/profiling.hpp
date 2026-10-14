#pragma once

// Profiling markers for the Tracy profiler (https://github.com/wolfpld/tracy).
//
// Build with `scons tracy=yes` to send data to the Tracy profiler app. In a normal build
// TRACY_ENABLE is not defined, Tracy is not even on the include path, and every macro
// below expands to nothing, so the markers cost nothing.
//
//   ZoneScopedN("name")        times the enclosing scope and shows it on the timeline
//   TracyPlot("name", value)   adds a value to a graph in the profiler
//   ECONOMY_COUNT_SEARCH()     counts one planner search for the "searches per frame" graph
//   ECONOMY_FRAME_END()        marks the end of a frame; called once per frame by the EconomyManager

#ifdef TRACY_ENABLE

#include <tracy/Tracy.hpp>

#include <cstdint>

namespace economy_profiling {
inline int64_t searches_this_frame = 0;
} // namespace economy_profiling

#define ECONOMY_COUNT_SEARCH() (++economy_profiling::searches_this_frame)
#define ECONOMY_FRAME_END()                                                                   \
	do {                                                                                      \
		TracyPlot("planner searches per frame", economy_profiling::searches_this_frame);       \
		economy_profiling::searches_this_frame = 0;                                           \
		FrameMark;                                                                            \
	} while (0)

#else

#define ZoneScoped
#define ZoneScopedN(name)
#define TracyPlot(name, value)
#define FrameMark
#define ECONOMY_COUNT_SEARCH()
#define ECONOMY_FRAME_END()

#endif
