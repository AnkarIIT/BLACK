#ifndef WASMEXTENSIONSUPPORT_H
#define WASMEXTENSIONSUPPORT_H

#include <QObject>

// Placeholder implementation. WebAssembly-based extension support (loading and
// instantiating .wasm extension modules) is not implemented in this build.
//
// isSupported() always reports false: no .wasm module is ever fetched,
// validated or instantiated.
class WasmExtensionSupport : public QObject
{
    Q_OBJECT

public:
    explicit WasmExtensionSupport(QObject *parent = nullptr);
    ~WasmExtensionSupport() override;

    // Always false: no WebAssembly extension runtime exists in this build.
    Q_INVOKABLE bool isSupported() const;
};

#endif // WASMEXTENSIONSUPPORT_H
