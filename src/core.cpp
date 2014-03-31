/**
 * \file core.cpp
 * \author Thibault Schueller <thibault.schueller@islog.com>
 * \brief module managing class
 */

#include "core.hpp"

#include <thread>
#include <iostream>

#include "modules/journallogger.hpp"
#include "hardware/hwmanager.hpp"

#include "hardware/device/gpio.hpp" // FIXME Debug
#include "signal/signalhandler.hpp"
#include "exception/signalexception.hpp"

Core::Core()
:   _isRunning(true),
    _hwManager(nullptr)
{}

Core::~Core() {}

Core::Core(const Core& /*other*/) {}

Core& Core::operator=(const Core& /*other*/)
{
    return (*this);
}

void Core::handleSignal(int /*signal*/)
{
    std::lock_guard<std::mutex> lg(_runMutex);
    if (_isRunning)
    {
        dispatchEvent(Event("caught signal", "Core"));
        _isRunning = false;
    }
}

void Core::run()
{
    load();
    dispatchEvent(Event("starting", "Core"));
    _runMutex.lock();
    while (_isRunning)
    {
        _runMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        dispatchEvent(Event("alive", "Core"));
        _runMutex.lock();
    }
    _runMutex.unlock();
    dispatchEvent(Event("exiting", "Core"));
    unload();
}

void Core::load()
{
    _hwManager = new HWManager;
    GPIO* gpio = _hwManager->reserveGPIO(12);
    gpio->getPinNo();
    _loggerModules.push_front(new JournalLogger(Event::Debug));

    try
    {
        SignalHandler::registerCallback(this);
    }
    catch (const SignalException& e)
    {
        dispatchEvent(Event(e.what(), "Core"));
    }
}

void Core::unload()
{
    for (auto logger : _loggerModules)
        delete logger;
    _loggerModules.clear();

    delete _hwManager;
    _hwManager = nullptr;
}

void Core::dispatchEvent(const Event& event)
{
    for (auto logger : _loggerModules)
        logger->sendEvent(event);
}