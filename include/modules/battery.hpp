#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <fmt/format.h>
#include <sys/inotify.h>
#include <algorithm>
#include "util/chrono.hpp"
#include "ALabel.hpp"

namespace waybar::modules {

namespace fs = std::filesystem;

class Battery : public ALabel {
  public:
    Battery(const std::string, const Json::Value&);
    ~Battery();
    auto update(const std::string) -> void;
  private:
    static inline const fs::path data_dir_ = "/sys/class/power_supply/";
  
    void worker(const std::string);

    util::SleeperThread thread_;
    std::vector<fs::path> batteries_;
    int fd_;
};

}
