#pragma once

class Test {
public:
  /// @brief test Udev rules inspection
  void Run1();

  /**
   * @brief  test daemon CheckConfig
   * @details to pass this test must have root privileges
   */
  void Run2();

  /**
   * @brief  test daemon CheckConfig find config file for daemon
   * @details to pass this test usbguard package must be installed
   */
  void Run3();

  /**
   * @brief  Parse config file to find path to rules.file
   * @details to pass this test usbguard package must be installed
   * @warning Needs root privileges
   */
  void Run4();

  /**
   * @brief  Parse config file to find allowed users
   * @details to pass this test usbguard package must be installed with defualt
   * config
   * @warning Needs root privileges
   */
  void Run5();
};