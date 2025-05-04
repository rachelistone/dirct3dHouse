Step 1: Use the Right Project Template
Make sure you're using:

C++ → Windows Desktop Application (not Console App, not Empty Project).

If you used an Empty Project, the default WinMain/Windows setup might be missing runtime support.

Step 2: Add Required Linker Dependencies
Right-click your project > Properties.

Go to:

Configuration Properties > Linker > Input > Additional Dependencies

Add this (append to what's already there):

d3d11.lib;d3dcompiler.lib;dxgi.lib;dxguid.lib;user32.lib;gdi32.lib;

Step 3: Set Correct Subsystem and Entry Point
Still in Project Properties:
Go to:
Linker > System > Subsystem → Make sure it's set to:
✅ Windows (/SUBSYSTEM:WINDOWS)

Go to:
Linker > Advanced > Entry Point → Set to:
✅ WinMainCRTStartup

This tells the linker to use your WinMain instead of main.