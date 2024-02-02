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
};