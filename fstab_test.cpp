#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libudev.h>
#include <memory>
#include <string>

void printDeviceProperties(std::shared_ptr<udev_device> device);
bool mountBlock(const char *block);

int main(int argrc, char *argc[]) {

  std::unique_ptr<udev, decltype(&udev_unref)> udev(udev_new(), udev_unref);
  if (!udev) {
    std::cerr << "Can't create udev object";
  }
  std::unique_ptr<udev_monitor, decltype(&udev_monitor_unref)> monitor(
      udev_monitor_new_from_netlink(udev.get(), "udev"), &udev_monitor_unref);
  udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "usb", NULL);
  udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "block", NULL);
  udev_monitor_enable_receiving(monitor.get());
  int fd = udev_monitor_get_fd(monitor.get());

  while (true) {
    struct timeval tv; // secs and microsecs
    fd_set fds;        // file descriptors set
    FD_ZERO(&fds);     // clear fds
    FD_SET(fd, &fds);  // adds the file descriptor fd to set
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int ret = select(fd + 1, &fds, NULL, NULL, &tv);
    // TODO handle errors
    if (ret > 0 &&
        FD_ISSET(fd, &fds)) { // if a file descriptor is still present in a set
      std::shared_ptr<udev_device> device(
          udev_monitor_receive_device(monitor.get()), &udev_device_unref);
      if (device) {
        std::cout << "Gotcha !" << std::endl;
        //  printDeviceProperties(device);

        const char *action =
            udev_device_get_property_value(device.get(), "ACTION");
        if (action) {
          std::cout << "Action: " << action << std::endl;
        }

        // vendor
        const char *vendor_id =
            udev_device_get_property_value(device.get(), "ID_VENDOR_ID");
        if (vendor_id) {
          std::cout << "VENDOR_ID = " << vendor_id << std::endl;
        }

        // model
        const char *model_id =
            udev_device_get_property_value(device.get(), "ID_MODEL_ID");
        if (model_id) {
          std::cout << "MODEL_ID = " << model_id << std::endl;
        }
        std::cout << std::endl << std::endl;

        if (action && vendor_id && model_id && strcmp(vendor_id, "8564") == 0 &&
            strcmp(model_id, "1000") == 0) {
          std::cout << "We have found the chosen device!" << std::endl;

          const char *subsystem =
              udev_device_get_property_value(device.get(), "SUBSYSTEM");
          if (subsystem && strcmp(subsystem, "block") == 0 && action &&
              strcmp(action, "add") == 0) {
            std::cout << "found block" << std::endl;
            const char *block =
                udev_device_get_property_value(device.get(), "DEVNAME");
            if (block) {
              std::cout << "dev = " << block << std::endl;
              mountBlock(block);
            }
          }
        }
      }
    }
  }
  return 0;
}

void printDeviceProperties(std::shared_ptr<udev_device> device) {
  struct udev_list_entry *properties =
      udev_device_get_properties_list_entry(device.get());
  if (!properties)
    return;
  struct udev_list_entry *entry;
  udev_list_entry_foreach(entry, properties) {
    const char *name = udev_list_entry_get_name(entry);
    const char *value = udev_list_entry_get_value(entry);
    if (name && value) {
      std::cout << name << " = " << value << std::endl;
    }
  }
}

bool mountBlock(const char *block) {
  std::string target = "/home/test";
  target += "/";
  target += std::filesystem::path(block).filename();
  std::string fs = "auto";

  if (!std::filesystem::exists(target)) {
    if (std::filesystem::create_directory(target)) {
      std::cout << "Folder created successfully." << std::endl;
    } else {
      std::cerr << "Failed to create folder." << std::endl;
    }
  }

  std::ofstream fstabFile("/etc/fstab", std::ios::app);
  if (!fstabFile.is_open()) {
    std::cerr << "Failed to open /etc/fstab for writing." << std::endl;
    return false;
  }
  // Write the new entry to /etc/fstab
  std::string options = "uid=1000,gid=1000,umask=007";
  fstabFile << block << "\t" << target << "\t" << fs << "\t" << options
            << "\t0\t0" << std::endl;
  // Close the file
  fstabFile.close();

  std::cout << "Rule added to /etc/fstab successfully." << std::endl;
  return true;
}

// blkid_probe pr = blkid_new_probe_from_filename(block);
//   const char* value = nullptr;
//   if (pr) {
//        if (blkid_do_probe(pr) == 0) {
//            if (blkid_probe_lookup_value(pr, "TYPE", &value, NULL) == 0) {
//                std::cout << "Filesystem type of " << block << " is " << value
//                << std::endl;
//            } else {
//                std::cerr << "Failed to determine filesystem type" <<
//                std::endl;
//            }
//        }
//        blkid_free_probe(pr);
//    } else {
//        std::cerr << "Failed to create probe for " << block << std::endl;
//    }
