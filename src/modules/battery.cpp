#include "modules/battery.hpp"

waybar::modules::Battery::Battery(const std::string name, const Json::Value& config)
  : ALabel(config, "{capacity}%")
{
  try {
    for (auto &node : fs::directory_iterator(data_dir_)) {
      if (fs::is_directory(node) && fs::exists(node / "capacity")
        && fs::exists(node / "status") && fs::exists(node / "uevent")) {
        batteries_.push_back(node);
      }
    }
  } catch (fs::filesystem_error &e) {
    throw std::runtime_error(e.what());
  }
  if (batteries_.empty()) {
    throw std::runtime_error("No batteries.");
  }
  fd_ = inotify_init1(IN_CLOEXEC);
  if (fd_ == -1) {
    throw std::runtime_error("Unable to listen batteries.");
  }
  for (auto &bat : batteries_) {
    inotify_add_watch(fd_, (bat / "uevent").c_str(), IN_ACCESS);
  }
  label_.set_name("battery");
  worker(name);
}

waybar::modules::Battery::~Battery()
{
  close(fd_);
}

void waybar::modules::Battery::worker(const std::string name)
{
  // Trigger first values
  update(name);
  thread_ = [this] {
    struct inotify_event event = {0};
    int nbytes = read(fd_, &event, sizeof(event));
    if (nbytes != sizeof(event)) {
      return;
    }
    dp.emit();
  };
}

auto waybar::modules::Battery::update(std::string name) -> void
{
  try {
    if (name == "battery") { // Combine all batteries
      uint16_t total = 0;
      std::string status;
      for (auto &bat : batteries_) {
        uint16_t capacity;
        std::string _status;
        std::ifstream(bat / "capacity") >> capacity;
        std::ifstream(bat / "status") >> _status;
        if (_status != "Unknown") {
          status = _status;
        }
        total += capacity;
      }

      uint16_t capacity = total / batteries_.size();
      label_.set_text(fmt::format(format_, fmt::arg("capacity", capacity),
        fmt::arg("icon", getIcon(capacity))));
      label_.set_tooltip_text(status);
      bool charging = status == "Charging";
      if (charging) {
        label_.get_style_context()->add_class("charging");
      } else {
        label_.get_style_context()->remove_class("charging");
      }
      auto critical = config_["critical"] ? config_["critical"].asUInt() : 15;
      if (capacity <= critical && !charging) {
        label_.get_style_context()->add_class("warning");
      } else {
        label_.get_style_context()->remove_class("warning");
      }
    } else { // Display only one battery
      int batid = std::stoi(name.erase(0, 7));
      std::string status;
      uint16_t capacity;
      std::ifstream(batteries_[batid] / "capacity") >> capacity;
      std::ifstream(batteries_[batid] / "status") >> status;

      label_.set_text(fmt::format(format_, fmt::arg("capacity", capacity),
        fmt::arg("icon", getIcon(capacity))));
      label_.set_tooltip_text(status);
      bool charging = status == "Charging";
      if (charging) {
        label_.get_style_context()->add_class("charging");
      } else {
        label_.get_style_context()->remove_class("charging");
      }
      auto critical = config_["critical"] ? config_["critical"].asUInt() : 15;
      if (capacity <= critical && !charging) {
        label_.get_style_context()->add_class("warning");
      } else {
        label_.get_style_context()->remove_class("warning");
      }
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}
