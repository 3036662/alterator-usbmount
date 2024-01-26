#include <iostream>
#include <libudev.h>
#include <cstring>

void printDeviceProperties(struct udev_device *device);

int main(int argrc, char *argc[]) {
  struct udev *udev;
  struct udev_monitor *monitor;

  udev = udev_new(); // Create, acquire and release a udev context object
  if (!udev) {
    std::cerr << "Can't create udev object";
  }
  monitor = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", NULL);
  udev_monitor_enable_receiving(monitor);
  int fd = udev_monitor_get_fd(monitor);

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
      struct udev_device *device = udev_monitor_receive_device(monitor);
      if (device) {
        std::cout << "Gotcha !" << std::endl;
        printDeviceProperties(device);

        const char *action = udev_device_get_property_value(device, "ACTION");
        if (action) {
          std::cout << "Action: " << action << std::endl;
        }

        // vendor    
        const char *vendor_id = udev_device_get_property_value(device, "ID_VENDOR_ID");
        if (vendor_id) {
          std::cout << "VENDOR_ID = " << vendor_id << std::endl;
        }

        // model
        const char* model_id = udev_device_get_property_value(device, "ID_MODEL_ID");
        if (model_id) {
          std::cout << "MODEL_ID = " << model_id << std::endl;
        }
        std::cout << std::endl << std::endl;
        
        if (action && vendor_id && model_id && strcmp(vendor_id,"8564")==0 && strcmp(model_id,"1000")==0){
            std::cout << "We have found the chosen device!" << std::endl;
            if (int err=udev_device_set_sysattr_value(device,"OWNER", "oleg") < 0){
                std::cerr << "Can't change the owner..." << err <<std::endl;
            }
            else {
                std::cout  << "Change owner. Success." <<std::endl;
            }
        }

        udev_device_unref(device);
      }
    }
  }
  udev_monitor_unref(monitor);
  udev_unref(udev);

  return 0;
}

void printDeviceProperties(struct udev_device *device) {
  struct udev_list_entry *properties =
      udev_device_get_properties_list_entry(device);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, properties) {
    const char *name = udev_list_entry_get_name(entry);
    const char *value = udev_list_entry_get_value(entry);
    std::cout << name << " = " << value << std::endl;
  }
}

// sd_event *event = nullptr;
// sd_device_monitor *monitor = nullptr;

// // reference to default event loop
// int res = sd_event_default(&event);
// if (res < 0){
//     std::cerr << "Failed to create event loop" << std::endl;
//     return 1;
// }

// res = sd_device_monitor_new(&monitor);
// if (res < 0){
//     std::cerr << "Failed to create device monitor" << std::endl;
//     sd_event_unref(event);
//     return 1;
// }

// res = sd_device_monitor_attach_event(monitor,event);
// if (res < 0){
//     std::cerr << "Failed to attach device monitor to the event loop" <<
//     std::endl; sd_device_monitor_unref(monitor); sd_event_unref(event);
//     return 1;
// }

// // Start the monitor
// res = sd_device_monitor_enable_receiving(monitor, true);
