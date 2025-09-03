// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved

#ifndef AIE_PROFILE_IMPL_H
#define AIE_PROFILE_IMPL_H

#include <thread>

#include "aie_profile_metadata.h"
#include "xdp/profile/plugin/vp_base/vp_base_plugin.h"

namespace xdp {

  // AIE profile configurations can be done in different ways depending
  // on the platform.  For example, platforms like the VCK5000 or
  // discovery platform, where the host code runs on the x86 and the AIE
  // is not directly accessible, will require configuration be done via
  // PS kernel. 
  class AieProfileImpl
  {

  protected:
    VPDatabase* db = nullptr;
    std::unique_ptr<AieProfileMetadata> metadata;
    std::atomic<bool> threadCtrl;
    std::unique_ptr<std::thread> thread;
    uint64_t deviceID;

  public:
    AieProfileImpl(VPDatabase* database, std::unique_ptr<AieProfileMetadata> metadata, uint64_t deviceID)
      : db(database),
        metadata(std::move(metadata)),
        threadCtrl(false),
        thread(nullptr),
        deviceID(deviceID)
    {}

    AieProfileImpl() = delete;
    virtual ~AieProfileImpl() {};

    virtual void updateDevice() = 0;

    virtual void startPoll(const uint32_t index) = 0;
    virtual void continuePoll(const uint32_t index) = 0;
    virtual void poll(const uint32_t index) = 0;
    virtual void endPoll() = 0;

    virtual void freeResources() = 0;

    uint64_t getDeviceID() const { return deviceID; }
  };

} // namespace xdp

#endif