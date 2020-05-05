# Cemu Patcher
DLL hook for Cemu to redirect nintendo.net requests to Pretendo

# How to use
Clone this project and build with VS. Place the compiled DLL in the same folder as Cemu with the name `dbghelp.dll` or `dbgcore.dll`. If patched correctly you will see an alert

# Use with CemuHook by Rajkosto
At this point in time this hook does not work along side CemuHook. This is because of an issue with getting both DLLs to load at the same time. Cemu will autoload several DLLs which is how hooking works, CemuHook makes use of `dbghelp`. However there is another dbg DLL, `dbgcore`, which also works. Trying to use both DLLs, though, (one using `dbgcore` and one using `dbghelp`) will fail. The DLL named `dbghelp` will always load and the DLL named `dbgcore` will be ignored

When using this patch, `dbgcore` will be loaded from System32. When using CemuHook `dbgcore` will never be loaded and instead `dbghelp` loads twice (once from System32 and once from the Cemu folder)

I have not found a way around this, though I would very much like to