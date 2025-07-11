// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2023-2025 Advanced Micro Devices, Inc. - All rights reserved
#define XRT_CORE_COMMON_SOURCE
#include "core/common/xdp/profile.h"

#include "core/common/config_reader.h"
#include "core/common/dlfcn.h"
#include "core/common/module_loader.h"
#include "core/common/message.h"
#include <functional>
#include <sstream>

#ifdef _WIN32
#pragma warning( disable : 4996 ) /* Disable warning for getenv */
#endif

// An anonymous namespace to hold a common set of blank functions
// for all modules that don't require specialization and the common
// load and update function
namespace {
  static void register_callbacks_empty(void*)
  {
  }

  static void warning_callbacks_empty()
  {
  }

  template <typename Callable1, typename Callable2, typename Callable3,
            typename... Args>
  static inline void load_once_and_update(Callable1 &&ini_check,
                                          Callable2 &&load_function,
                                          Callable3 &&update_function,
                                          const std::string &load_fail_msg,
                                          const std::string &update_fail_msg,
                                          Args &&...update_args) {
    if (ini_check()) {
      try {
        load_function();
      } catch (const std::exception &e) {
        std::stringstream msg;
        msg << load_fail_msg << e.what();
        xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                                msg.str());
      }
      try {
        update_function(update_args...);
      } catch (const std::exception &e) {
        std::stringstream msg;
        msg << update_fail_msg << e.what();
        xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                                msg.str());
      }
    }
  }
} // end anonymous namespace

// This file makes the connections between all xrt_coreutil level hooks
// to the corresponding xdp plugins.  It is responsible for loading all of
// modules.

namespace xrt_core::xdp::core {
  void
  load_core()
  {
    static xrt_core::module_loader xdp_core_loader("xdp_core",
                                                    register_callbacks_empty,
                                                    warning_callbacks_empty);
  }
}

namespace xrt_core::xdp::aie::profile {

std::function<void (void*, bool)> update_device_cb;
std::function<void (void*)> end_poll_cb;

void 
register_callbacks(void* handle)
{  
    using ftype = void (*)(void*);
    using utype = void (*)(void*, bool);

    update_device_cb = reinterpret_cast<utype>(xrt_core::dlsym(handle, "updateAIECtrDevice"));
    end_poll_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "endAIECtrPoll"));
}

void 
load()
{
  static xrt_core::module_loader xdp_aie_loader("xdp_aie_profile_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

void
load_xdna()
{
  static xrt_core::module_loader xdp_aie_loader("xdp_aie_profile_plugin_xdna",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void 
update_device(void* handle, bool hw_context_flow)
{
  if (update_device_cb)
    update_device_cb(handle, hw_context_flow);
}

void 
end_poll(void* handle)
{
  if (end_poll_cb)
    end_poll_cb(handle);
}

} // end namespace xrt_core::xdp::aie::profile

namespace xrt_core::xdp::aie::debug {

std::function<void (void*)> update_device_cb;
std::function<void (void*)> end_debug_cb;

void 
register_callbacks(void* handle)
{  
  #if defined(XDP_CLIENT_BUILD) || defined(XDP_VE2_BUILD)
    using ftype = void (*)(void*);

    end_debug_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "endAIEDebugRead"));
    update_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "updateAIEDebugDevice"));
  #else 
    (void)handle;
  #endif
}


void 
load()
{
  static xrt_core::module_loader xdp_aie_debug_loader("xdp_aie_debug_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void 
update_device(void* handle)
{
  if (update_device_cb)
    update_device_cb(handle);
}

void
end_debug(void* handle)
{
  if (end_debug_cb)
    end_debug_cb(handle);
}

} // end namespace xrt_core::xdp::aie::debug

namespace xrt_core::xdp::aie::status {

std::function<void (void*, bool)> update_device_cb;
std::function<void (void*)> end_status_cb;

void
register_callbacks(void* handle)
{
  #if defined(XDP_VE2_BUILD)
    using ftype = void (*)(void*);
    using utype = void (*)(void*, bool);
    
    update_device_cb = reinterpret_cast<utype>(xrt_core::dlsym(handle, "updateAIEStatusDevice"));
    end_status_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "endAIEStatusPoll"));
  #else
    (void)handle;
  #endif
}


void
load()
{
  static xrt_core::module_loader xdp_aie_status_loader("xdp_aie_status_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void
update_device(void* handle, bool hw_context_flow)
{
  if (update_device_cb)
    update_device_cb(handle, hw_context_flow);
}

void
end_status(void* handle)
{
  if (end_status_cb)
  end_status_cb(handle);
}

} // end namespace xrt_core::xdp::aie::status

namespace xrt_core::xdp::ml_timeline {

std::function<void (void*)> update_device_cb;
std::function<void (void*)> finish_flush_device_cb;

void
register_callbacks(void* handle)
{ 
  #if defined(XDP_CLIENT_BUILD) || defined(XDP_VE2_BUILD)
    using ftype = void (*)(void*);

    update_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "updateDeviceMLTmln"));
    finish_flush_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "finishflushDeviceMLTmln"));

  #else
    (void)handle;
  #endif

}


void
load()
{
  static xrt_core::module_loader xdp_loader("xdp_ml_timeline_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void
update_device(void* handle)
{
  if (update_device_cb)
    update_device_cb(handle);
}

void
finish_flush_device(void* handle)
{
  if (finish_flush_device_cb)
    finish_flush_device_cb(handle);
}

} // end namespace xrt_core::xdp::ml_timeline

namespace xrt_core::xdp::aie_pc {

std::function<void (void*)> update_device_cb;
std::function<void (void*)> finish_flush_device_cb;

void
register_callbacks(void* handle)
{ 
  #ifdef XDP_CLIENT_BUILD
    using ftype = void (*)(void*);

    update_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "updateDeviceAIEPC"));
    finish_flush_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "finishflushDeviceAIEPC"));
  #else
    (void)handle;
  #endif

}

void
load()
{
  static xrt_core::module_loader xdp_loader("xdp_aie_pc_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void
update_device(void* handle)
{
  if (update_device_cb)
    update_device_cb(handle);
}

void
finish_flush_device(void* handle)
{
  if (finish_flush_device_cb)
    finish_flush_device_cb(handle);
}

} // end namespace xrt_core::xdp::aie_pc

namespace xrt_core::xdp::pl_deadlock {

std::function<void (void*)> update_device_cb;
std::function<void (void*)> finish_flush_device_cb;

void
register_callbacks(void* handle)
{ 
  #ifdef XDP_CLIENT_BUILD
    (void)handle;	// Not supported on Client Devices.
  #else
    using ftype = void (*)(void*);

    update_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "updateDevicePLDeadlock"));
    finish_flush_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "flushDevicePLDeadlock"));
  #endif

}

void
load()
{
  static xrt_core::module_loader xdp_loader("xdp_pl_deadlock_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void
update_device(void* handle)
{
  if (update_device_cb)
    update_device_cb(handle);
}

void
finish_flush_device(void* handle)
{
  if (finish_flush_device_cb)
    finish_flush_device_cb(handle);
}

} // end namespace xrt_core::xdp::pl_deadlock

namespace xrt_core::xdp::aie::trace {

std::function<void (void*, bool)> update_device_cb;
std::function<void (void*)> end_trace_cb;


void 
register_callbacks(void* handle)
{  
    using ftype = void (*)(void*);
    using utype = void (*)(void*, bool);

    end_trace_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "finishFlushAIEDevice"));
    update_device_cb = reinterpret_cast<utype>(xrt_core::dlsym(handle, "updateAIEDevice"));
}

void 
load()
{
  static xrt_core::module_loader xdp_aie_trace_loader("xdp_aie_trace_plugin",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

void 
load_xdna()
{
  static xrt_core::module_loader xdp_aie_trace_loader("xdp_aie_trace_plugin_xdna",
                                                register_callbacks,
                                                warning_callbacks_empty);
}

// Make connections
void 
update_device(void* handle, bool hw_context_flow)
{
  if (update_device_cb)
    update_device_cb(handle, hw_context_flow);
}

void 
end_trace(void* handle)
{
  if (end_trace_cb)
    end_trace_cb(handle);
}

} // end namespace xrt_core::xdp::aie::trace

namespace xrt_core::xdp::aie::halt {

std::function<void (void*)> update_device_cb;
std::function<void (void*)> finish_flush_device_cb;

void
register_callbacks(void* handle)
{
  #if defined(XDP_CLIENT_BUILD) || defined(XDP_VE2_BUILD)
    using ftype = void (*)(void*);
    
    update_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "updateDeviceAIEHalt"));
    finish_flush_device_cb = reinterpret_cast<ftype>(xrt_core::dlsym(handle, "finishFlushDeviceAIEHalt"));
  #else
    (void)handle;
  #endif
}

void
load()
{
  static xrt_core::module_loader xdp_aie_halt_loader("xdp_aie_halt_plugin",
                                                     register_callbacks,
                                                     warning_callbacks_empty);
}

void
update_device(void* handle)
{
  if (update_device_cb)
    update_device_cb(handle);
}

void
finish_flush_device(void* handle)
{
  if (finish_flush_device_cb)
    finish_flush_device_cb(handle);
}

} // end namespace xrt_core::xdp::aie::halt

namespace xrt_core::xdp {

void 
update_device(void* handle, bool hw_context_flow)
{
#ifdef XDP_CLIENT_BUILD
  /* Adding the macro guard as the static instances of the following plugins
   * get created unnecessarily when the configs are enabled on Edge.
   */
  #ifdef _WIN32
  if (xrt_core::config::get_ml_timeline()
      || xrt_core::config::get_aie_profile()
      || xrt_core::config::get_aie_trace()
      || xrt_core::config::get_aie_debug()
      || xrt_core::config::get_aie_halt()
      || xrt_core::config::get_aie_pc()) {
    /* All the above plugins are dependent on xdp_core library. So,
     * explicitly load it to avoid library search issue in implicit loading.
     */
    try {
      xrt_core::xdp::core::load_core();
    } catch (const std::exception &e) {
      std::stringstream msg;
      msg << "Failed to load XDP Core library. Caught exception " << e.what();
      xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT", msg.str());
      return;
    }
  }
  #endif

  load_once_and_update(xrt_core::config::get_ml_timeline,
		       xrt_core::xdp::ml_timeline::load,
		       xrt_core::xdp::ml_timeline::update_device,
		       "Failed to load ML Timeline library. Caught exception ",
		       "Failed to setup for ML Timeline. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_halt,
		       xrt_core::xdp::aie::halt::load,
		       xrt_core::xdp::aie::halt::update_device,
		       "Failed to load AIE Halt library. Caught exception ",
		       "Failed to setup for AIE Halt. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_profile,
		       xrt_core::xdp::aie::profile::load,
		       xrt_core::xdp::aie::profile::update_device,
		       "Failed to load AIE Profile library. Caught exception ",
		       "Failed to setup for AIE Profile. Caught exception ",
		       handle,
		       hw_context_flow);

  load_once_and_update(xrt_core::config::get_aie_trace,
		       xrt_core::xdp::aie::trace::load,
		       xrt_core::xdp::aie::trace::update_device,
		       "Failed to load AIE Trace library. Caught exception ",
		       "Failed to setup for AIE Trace. Caught exception ",
		       handle,
		       hw_context_flow);

  load_once_and_update(xrt_core::config::get_aie_debug,
		       xrt_core::xdp::aie::debug::load,
		       xrt_core::xdp::aie::debug::update_device,
		       "Failed to load AIE Debug library. Caught exception ",
		       "Failed to setup for AIE Debug. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_pc,
		       xrt_core::xdp::aie_pc::load,
		       xrt_core::xdp::aie_pc::update_device,
		       "Failed to load AIE PC library. Caught exception ",
		       "Failed to setup for AIE PC. Caught exception ",
		       handle);

#elif defined(XDP_VE2_BUILD)

  load_once_and_update(xrt_core::config::get_ml_timeline,
		       xrt_core::xdp::ml_timeline::load,
		       xrt_core::xdp::ml_timeline::update_device,
		       "Failed to load ML Timeline library. Caught exception ",
		       "Failed to setup for ML Timeline. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_halt,
		       xrt_core::xdp::aie::halt::load,
		       xrt_core::xdp::aie::halt::update_device,
		       "Failed to load AIE Halt library. Caught exception ",
		       "Failed to setup for AIE Halt. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_trace,
           []() {
            if (xrt_core::config::get_xdp_mode() == "xdna") {
              xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                "xdp_mode config is set to XDNA. Hence, AIE Event Trace will be available only for XDNA device.");
              xrt_core::xdp::aie::trace::load_xdna();
            } else {
              xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                "xdp_mode config is set to ZOCL. Hence, AIE Event Trace will be available only for ZOCL device.");
              xrt_core::xdp::aie::trace::load();
            }
           },  
		       xrt_core::xdp::aie::trace::update_device,
		       "Failed to load AIE Trace library. Caught exception ",
		       "Failed to setup for AIE Trace. Caught exception ",
		       handle,
		       hw_context_flow);

  load_once_and_update(xrt_core::config::get_aie_debug,
		       xrt_core::xdp::aie::debug::load,
		       xrt_core::xdp::aie::debug::update_device,
		       "Failed to load AIE Debug library. Caught exception ",
		       "Failed to setup for AIE Debug. Caught exception ",
		       handle);

  load_once_and_update(xrt_core::config::get_aie_status,
		       xrt_core::xdp::aie::status::load,
		       xrt_core::xdp::aie::status::update_device,
		       "Failed to load AIE Status library. Caught exception ",
		       "Failed to setup for AIE Status. Caught exception ",
		       handle,
		       hw_context_flow);

  load_once_and_update(xrt_core::config::get_aie_profile,  
           []() {
            if (xrt_core::config::get_xdp_mode() == "xdna") {
              xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                "xdp_mode config is set to XDNA. Hence, profiling will be available only for XDNA device.");
              xrt_core::xdp::aie::profile::load_xdna();
            } else {
              xrt_core::message::send(xrt_core::message::severity_level::debug, "XRT",
                "xdp_mode config is set to ZOCL. Hence, profiling will be available only for ZOCL device.");
              xrt_core::xdp::aie::profile::load();
            }
           },  
           xrt_core::xdp::aie::profile::update_device,  
           "Failed to load AIE Profile library. Caught exception ",  
           "Failed to setup for AIE Profile. Caught exception ",
           handle,
	 	      hw_context_flow);

#else

  load_once_and_update(  
           []() {
            return xrt_core::config::get_pl_deadlock_detection() &&
              nullptr == std::getenv("XCL_EMULATION_MODE");  
           },  
           xrt_core::xdp::pl_deadlock::load,  
           xrt_core::xdp::pl_deadlock::update_device,  
           "Failed to load PL Deadlock Detection library. Caught exception ",  
           "Failed to setup for PL Deadlock Detection library. Caught exception ",  
           handle);  

  load_once_and_update(xrt_core::config::get_aie_trace,  
           xrt_core::xdp::aie::trace::load,
           xrt_core::xdp::aie::trace::update_device,
           "Failed to load AIE Trace library. Caught exception ",  
           "Failed to setup for AIE Trace. Caught exception ",
           handle,
           hw_context_flow);

  load_once_and_update(xrt_core::config::get_aie_profile,
           xrt_core::xdp::aie::profile::load,
           xrt_core::xdp::aie::profile::update_device,
           "Failed to load AIE Profile library. Caught exception ",
           "Failed to setup for AIE Profile. Caught exception ",
           handle,
           hw_context_flow);

  // Avoid warning until we've added support in all plugins
  (void)(hw_context_flow);
#endif
}

void 
finish_flush_device(void* handle)
{

#ifdef XDP_CLIENT_BUILD

  if (xrt_core::config::get_ml_timeline())
    xrt_core::xdp::ml_timeline::finish_flush_device(handle);
  if (xrt_core::config::get_aie_halt())
    xrt_core::xdp::aie::halt::finish_flush_device(handle);
  if (xrt_core::config::get_aie_profile())
    xrt_core::xdp::aie::profile::end_poll(handle);
  if (xrt_core::config::get_aie_trace())
    xrt_core::xdp::aie::trace::end_trace(handle);
  if (xrt_core::config::get_aie_debug())
    xrt_core::xdp::aie::debug::end_debug(handle);
  if (xrt_core::config::get_aie_pc())
    xrt_core::xdp::aie_pc::finish_flush_device(handle);

#elif defined(XDP_VE2_BUILD)

  if (xrt_core::config::get_aie_halt())
    xrt_core::xdp::aie::halt::finish_flush_device(handle);
  if (xrt_core::config::get_aie_trace())
    xrt_core::xdp::aie::trace::end_trace(handle);
  if (xrt_core::config::get_aie_debug())
    xrt_core::xdp::aie::debug::end_debug(handle);
  if (xrt_core::config::get_aie_status())
    xrt_core::xdp::aie::status::end_status(handle);
  if (xrt_core::config::get_ml_timeline())
    xrt_core::xdp::ml_timeline::finish_flush_device(handle);
  if (xrt_core::config::get_aie_profile())
    xrt_core::xdp::aie::profile::end_poll(handle);

#else

  if (xrt_core::config::get_pl_deadlock_detection()
      && nullptr == std::getenv("XCL_EMULATION_MODE")) {
    xrt_core::xdp::pl_deadlock::finish_flush_device(handle);
  }
  if (xrt_core::config::get_aie_trace())
    xrt_core::xdp::aie::trace::end_trace(handle);
  if (xrt_core::config::get_aie_profile())
    xrt_core::xdp::aie::profile::end_poll(handle);
#endif
}

} // end namespace xrt_core::xdp
