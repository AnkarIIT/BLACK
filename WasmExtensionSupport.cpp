#include "WasmExtensionSupport.h"

WasmExtensionSupport::WasmExtensionSupport(QObject *parent)
    : QObject(parent)
{
}

WasmExtensionSupport::~WasmExtensionSupport() = default;

bool WasmExtensionSupport::isSupported() const
{
    return false;
}