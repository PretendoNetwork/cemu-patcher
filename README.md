# Cemu Patcher
DLL hook for Cemu to redirect nintendo.net requests to Pretendo

# How to use
Clone this project and build with VS or download from the Releases page. Place the compiled `cemuhook.dll` in the same folder as Cemu. If patched correctly you will see an alert

# Use with CemuHook by Rajkosto
Due to loading issues with trying to load both CemuHook and cemu-patcher at the same time because of file names and DLL loading orders, cemu-patcher will search for a DLL named `true_cemuhook.dll` and inject it into the Cemu process if found. This is not officially supported by Rajkosto and may cause issues. Use at your own risk.