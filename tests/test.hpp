#pragma once

class Test {
public:
  /// @brief test Udev rules inspection
  void Run1();

  /**
   * @brief  test daemon CheckConfig
   * @details to pass this test systemctl enable usbguard
   * systemctl enable usbguard
   * systemctl start usbguard
   */
  void Run2();

  /**
   * @brief  test daemon CheckConfig find config file for daemon
   * @details to pass this test usbguard package must be installed
   */
  void Run3();
};