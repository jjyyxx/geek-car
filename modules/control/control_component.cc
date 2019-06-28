#include "modules/control/control_component.h"

#include <string>
#include "cyber/cyber.h"
#include "modules/control/Uart.h"
#include "modules/control/proto/chassis.pb.h"
#include "modules/control/proto/control.pb.h"

namespace apollo {
namespace control {

using apollo::control::Chassis;
using apollo::control::Control_Command;
using apollo::cyber::Time;

typedef union FLOAT_CONV {
  float f;
  char c[4];
} float_conv;

float BLEndianFloat(float value) {
  float_conv d1, d2;
  d1.f = value;
  d2.c[0] = d1.c[3];
  d2.c[1] = d1.c[2];
  d2.c[2] = d1.c[1];
  d2.c[3] = d1.c[0];
  return d2.f;
}

bool ControlComponent::Init() {
  Uart arduino_("ttyACM0");
  arduino_.SetOpt(9600, 8, 'N', 1);  //, int bits, char event, int stop);

  chassis_writer_ = node_->CreateWriter<Chassis>("/chassis");
  control_writer_ = node_->CreateWriter<Control_Command>("/control");

  TestCommand(arduino_);

  // collect chassis feedback
  chassis_feedback_ =
      cyber::Async(&ControlComponent::OnChassis, this, arduino_);
  return true;
}

/**
 * @brief read arduino and write to chassis channel
 *
 */
void ControlComponent::OnChassis(Uart arduino_) {
  while (!cyber::IsShutdown()) {
    static char buffer[100];
    memset(buffer, 0, 100);
    int ret = arduino_.Read(buffer, 100);
    if (ret != -1) {
      std::string s = buffer;
      AINFO << "chassis feedback : " << s;
      // std::cout << "Arduino says: " << std::endl << s << std::endl;

      //   auto proto_chassis = make_shared<Chassis>();
      //   proto_chassis->set_steer_angle();
      //   proto_chassis->set_throttle();
      //   proto_chassis->set_speed();
      //   chassis_writer_->Writer(proto_chassis);
    }
  }
}

void ControlComponent::GenerateCommand() {
  // write to channel
}

/**
 * @brief action method for control command
 *
 * @param arduino_
 * @param cmd
 */
void ControlComponent::Action(Uart arduino_, Control_Command& cmd) {
  while (!cyber::IsShutdown()) {
    if (!cmd.has_steer_angle() || !cmd.has_throttle()) {
      continue;
      cyber::SleepFor(std::chrono::milliseconds());
    }

    float steer_angle = cmd.steer_angle;
    float steer_throttle = cmd.throttle;
    ADEBUG << "control message, times: " << t << " steer_angle:" << steer_angle
           << " steer_throttle:" << steer_throttle;
    char protoco_buf[10];
    memcpy(protoco_buf, &steer_angle, 4);
    memcpy(protoco_buf + 4, &steer_throttle, 4);
    protoco_buf[8] = 0x0d;
    protoco_buf[9] = 0x0a;
    arduino_.Write(protoco_buf, 10);
    t += 0.05;
  }
}

void ControlComponent::TestCommand(Uart arduino_) {
  double t = 0.0;
  while (1) {
    // TODO read from generated control command
    float steer_angle = static_cast<float>(40 * sin(t));
    float steer_throttle = static_cast<float>(20 * cos(t));
    ADEBUG << "control message, times: " << t << " steer_angle:" << steer_angle
           << " steer_throttle:" << steer_throttle;
    char protoco_buf[10];
    // float steer = BLEndianFloat(steer_angle);
    // float throttle = BLEndianFloat(steer_throttle);
    memcpy(protoco_buf, &steer_angle, 4);
    memcpy(protoco_buf + 4, &steer_throttle, 4);
    protoco_buf[8] = 0x0d;
    protoco_buf[9] = 0x0a;
    arduino_.Write(protoco_buf, 10);

    // std::string log_control = protoco_buf;
    // ADEBUG << "sent control origin message" << log_control;

    t += 0.05;
    // std::this_thread::sleep_for(std::chrono::milliseconds(50000));
  }
}

}  // namespace control
}  // namespace apollo
