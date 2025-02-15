#include "filedialog.h"
#include <Cocoa/Cocoa.h>

namespace emulator::frontend
{

std::string FileDialog::Open()
{
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:NO];

    if ([openPanel runModal] == NSModalResponseOK) {
        NSURL* selection = [[openPanel URLs] objectAtIndex:0];
        return std::string([[selection path] UTF8String]);
    }
    
    return "";
}

}; // namespace emulator::frontend
