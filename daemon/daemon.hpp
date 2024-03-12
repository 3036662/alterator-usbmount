#pragma once

class Daemon {
public:
  static Daemon &instance() {
    static Daemon instance;
    return instance;
  }

  bool IsRunning();

private:
  Daemon();
  Daemon(Daemon const &) = delete;
  Daemon(Daemon const &&) = delete;
  void operator=(Daemon const &) = delete;

  static void ConnectToDBus();

  static void SignalHandler(int signal);
  static void Reload(){};

  bool is_running_;
  bool reload_;
};