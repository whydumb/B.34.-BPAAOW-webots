// Copyright 1996-2024 Cyberbotics Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RECEIVER_HPP
#define RECEIVER_HPP

#include <webots/Device.hpp>

namespace webots {
  class Receiver : public Device {
  public:
    explicit Receiver(const std::string &name) : Device(name) {}  // Use Robot::getReceiver() instead
    explicit Receiver(WbDeviceTag tag) : Device(tag) {}
    virtual ~Receiver() {}
    enum { CHANNEL_BROADCAST = -1 };
    virtual void enable(int samplingPeriod);
    virtual void disable();
    int getSamplingPeriod() const;
    virtual void setChannel(int channel);
    int getChannel() const;
    int getQueueLength() const;
    virtual void nextPacket();
    int getDataSize() const;
    const void *getData() const;
    double getSignalStrength() const;
    const double *getEmitterDirection() const;
  };
}  // namespace webots

#endif  // RECEIVER_HPP
