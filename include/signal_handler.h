#pragma once

#include "posix_includes.h"

#include <functional>

class signal_handler final {
   public:
    enum struct signal {
        sigfpe = SIGFPE,
        sighup = SIGHUP,
        sigill = SIGILL,
        sigint = SIGINT,
        sigkill = SIGKILL,
        sigpipe = SIGPIPE,
        sigquit = SIGQUIT,
        sigsegv = SIGSEGV,
        sigstop = SIGSTOP,
        sigterm = SIGTERM,
        sigtstp = SIGTSTP,
        sigttin = SIGTTIN,
        sigttou = SIGTTOU,
        sigusr1 = SIGUSR1,
        sigusr2 = SIGUSR2,
        sigpoll = SIGPOLL,
        sigprof = SIGPROF,
        sigsys = SIGSYS,
        sigtrap = SIGTRAP,
        sigurg = SIGURG,
        sigvtalrm = SIGVTALRM,
        sigxcpu = SIGXCPU,
        sigxfsz = SIGXFSZ
    };

    signal_handler() = default;
    ~signal_handler() = default;

    bool disable_for_current_thread();
    static int install_signal_handler(signal sig, void (*handler)(int), std::initializer_list<signal> ignored_signals);

   private:
};
