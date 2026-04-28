/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>
#include <unordered_map>
#include <signal.h>

#ifndef __SIGRTMIN
#define __SIGRTMIN 32
#endif

static const constexpr int DEBUGGER_SIGNAL = (__SIGRTMIN + 3);
static const constexpr uint32_t kDefaultSignalPendingTries = 10;
static const constexpr uint32_t kDefaultRegistrationDelayMs = 500;

struct SignalHandler {
    static std::shared_ptr<SignalHandler> getInstance();
    static void setClientCallback(std::function<void(int, pid_t, uid_t)> cb);
    static void asyncRegister(int signal);
#if defined(FEATURE_IPQ_OPENWRT) || defined(LINUX_ENABLED)
    static void invokeDefaultHandler(std::shared_ptr<struct sigaction> sAct,
                              int code, siginfo_t *si, void *sc);
    static void customSignalHandler(int code, siginfo_t *si, void *sc);
#else
    static void invokeDefaultHandler(std::shared_ptr<struct sigaction> sAct,
                              int code, struct siginfo *si, void *sc);
    static void customSignalHandler(int code, struct siginfo *si, void *sc);
#endif
    static std::vector<int> getRegisteredSignals();
    void registerSignalHandler(std::vector<int> signalsToRegister);
    static std::mutex sDefaultSigMapLock;
    static std::unordered_map<int, std::shared_ptr<struct sigaction>> sDefaultSigMap;
    static std::function<void(int, pid_t, uid_t)> sClientCb;
    static std::mutex sAsyncRegisterLock;
    static std::future<void> sAsyncHandle;
};

#endif
