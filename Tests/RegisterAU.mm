#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstring>
#include <iostream>

bool registerLocalAU(const char* path)
{
    auto url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(path), static_cast<CFIndex>(strlen(path)), true);
    if (url == nullptr)
        return false;
    auto bundle = CFBundleCreate(nullptr, url);
    CFRelease(url);
    if (bundle == nullptr)
        return false;
    CFErrorRef error = nullptr;
    if (!CFBundleLoadExecutableAndReturnError(bundle, &error))
    {
        if (error != nullptr)
        {
            CFShow(error);
            CFRelease(error);
        }
        CFRelease(bundle);
        return false;
    }
    auto factory = reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(bundle, CFSTR("Small_TimerAUFactory")));
    AudioComponentDescription description { 'aufx', 'Smtr', 'Smin', 0, 0 };
    UInt32 version = 0;
    auto components = static_cast<CFArrayRef>(CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("AudioComponents")));
    if (components != nullptr && CFArrayGetCount(components) > 0)
    {
        auto details = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(components, 0));
        auto number = static_cast<CFNumberRef>(CFDictionaryGetValue(details, CFSTR("version")));
        if (number != nullptr)
            CFNumberGetValue(number, kCFNumberSInt32Type, &version);
    }
    const auto component = factory != nullptr ? AudioComponentRegister(&description, CFSTR("Small Instruments: Small Timer"), version, factory) : nullptr;
    const bool success = component != nullptr;
    if (!success)
        std::cerr << "AU registration failed: factory=" << (factory != nullptr) << ", bundle=" << path << '\n';
    CFRelease(bundle);
    return success;
}
