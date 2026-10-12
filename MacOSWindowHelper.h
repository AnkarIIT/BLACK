#ifndef MACOSWINDOWHELPER_H
#define MACOSWINDOWHELPER_H

#include <QtGlobal>

class QWidget;

// Placeholder implementation. Native macOS window configuration (traffic-light
// button placement, full-size content view, title-bar transparency) is not
// implemented in this build; configureMacOSWindow() is a no-op on every
// platform.
namespace MacOSWindowHelper {

// No-op. Does nothing at all on any platform, including macOS.
void configureMacOSWindow(QWidget *window);

} // namespace MacOSWindowHelper

#endif // MACOSWINDOWHELPER_H