#include "MacOSWindowHelper.h"

#include <QWidget>

namespace MacOSWindowHelper {

void configureMacOSWindow(QWidget *window)
{
#if defined(Q_OS_MACOS)
    // Placeholder: a real implementation would install the NSWindow
    // full-size-content-view style mask, reposition the standard
    // close/minimise/zoom buttons and make the title bar transparent.
    // None of that is done here, so the window keeps Qt's default chrome.
    Q_UNUSED(window)
#else
    // Placeholder: this helper is only meaningful on macOS. On other platforms
    // it does nothing (BrowserWindow guards the call site, but stay safe).
    Q_UNUSED(window)
#endif
}

} // namespace MacOSWindowHelper
