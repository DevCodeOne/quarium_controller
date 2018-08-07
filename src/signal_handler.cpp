#include "signal_handler.h"

#include <cstring>

int signal_handler::install_signal_handler(signal_handler::signal sig, void (*handler)(int),
                                           std::initializer_list<signal> ignored_signals) {
    struct sigaction action;
    std::memset(&action, 0, sizeof(struct sigaction));
    sigemptyset(&action.sa_mask);

    for (auto current_signal : ignored_signals) {
        sigaddset(&action.sa_mask, (int)current_signal);
    }

    action.sa_handler = handler;

    return sigaction((int)sig, &action, nullptr);
}

bool signal_handler::disable_for_current_thread() {
    sigset_t mask;
    sigfillset(&mask);
    return pthread_sigmask(SIG_BLOCK, &mask, nullptr) >= 0;
}
