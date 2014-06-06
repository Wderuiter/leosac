/**
 * \file modulemanager.cpp
 * \author Thibault Schueller <ryp.sqrt@gmail.com>
 * \brief ModuleManager class implementation
 */

#include "modulemanager.hpp"

#include "tools/unixfs.hpp"
#include "tools/log.hpp"
#include "dynlib/dynamiclibrary.hpp"
#include "exception/moduleexception.hpp"
#include "exception/dynlibexception.hpp"

#include "modules/iauthmodule.hpp"
#include "modules/iloggermodule.hpp"

ModuleManager::ModuleManager()
{
}

void ModuleManager::loadLibraries(const std::list<std::string>& directories)
{
    std::string     libname;

    for (auto& folder : directories)
    {
        UnixFs::FileList fl = UnixFs::listFiles(folder, ".so");
        for (auto& path : fl)
            loadLibrary(path);
    }
}

void ModuleManager::unloadLibraries()
{
    for (auto& lib : _dynlibs)
    {
        try {
            lib.second->close();
        }
        catch (const DynLibException& e) {
            throw (ModuleException(e.what()));
        }
        delete lib.second;
    }
    _dynlibs.clear();
}

IModule* ModuleManager::loadModule(ICore& core, const std::string& libname, const std::string& alias)
{
    DynamicLibrary*     lib = _dynlibs.at(libname);
    IModule::InitFunc   func;
    IModule*            module = nullptr;

    if (!lib)
        throw (ModuleException("Invalid source library"));
    if (_modules.count(alias) > 0)
        throw (ModuleException("A module named \'" + alias + "\' already exists"));
    try {
        void* s = lib->getSymbol("getNewModuleInstance");
        *reinterpret_cast<void**>(&func) = s;
        module = func(core, alias);
    }
    catch (const DynLibException& e) {
        delete module; // FIXME is it safe ?
        throw (ModuleException(e.what()));
    }
    _modules[alias] = { libname, module };
    return (module);
}

void ModuleManager::unloadModules()
{
    for (const auto& module : _modules)
        delete module.second.instance;
    _modules.clear();
}

const std::map< std::string, ModuleManager::Module >& ModuleManager::getModules() const
{
    return (_modules);
}

void ModuleManager::loadLibrary(const std::string& path)
{
    std::string     libname(UnixFs::stripPath(path));
    DynamicLibrary* lib;

    if (_dynlibs.count(libname) > 0)
        throw (ModuleException("module already loaded (" + path + ')'));
    lib = new DynamicLibrary(path);
    try {
        lib->open();
    }
    catch (const DynLibException& e) {
        delete lib;
        throw (ModuleException(e.what()));
    }
    _dynlibs[libname] = lib;
}