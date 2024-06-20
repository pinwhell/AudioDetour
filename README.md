# Audio Detour

This project aims to customize audio device management on Windows using a custom DLL. The DLL intercepts and modifies how multimedia devices are enumerated and selected by hooking into the device creation process. Specifically, when an application requests a list of audio devices or a default audio device, the DLL substitutes the standard IMMDeviceEnumerator with DetourMultiMediaDevEnumer. This allows users to select their preferred audio device from available options, ensuring the choice is enforced within the specific application where the DLL is injected.

## Functionality Overview

-   **Hooking Mechanism**: The DLL hooks into system functions like `CoCreateInstance` to replace instances of `IMMDeviceEnumerator` with its custom implementation.
    
-   **Process Injection**: It hooks `ZwCreateUserProcess` to inject itself into newly created processes, ensuring that the customized functionality extends to new instances of applications.
    

## Purpose

This approach provides a flexible way to modify audio device selection behavior at a low level. It enables tailoring of audio device usage based on user preferences within specific applications. However, it's worth noting that for production environments, using Windows' native controls and APIs for managing multimedia devices may be more efficient and less intrusive than hooking system functions and injecting DLLs into processes.

## Usage

To use this DLL:

1.  **Compile**: Compile the DLL, it uses cmake build system
2.  **Injection**: Inject the compiled DLL into the target application's process. Tools like `injector.exe` or manual injection methods can be used for this purpose.
3.  **Customization**: Once injected, the DLL will modify how audio devices are enumerated and selected within the application.

## Notes

-   Make sure you have the necessary permissions and approvals before modifying system behavior with this approach.

## License

This project is licensed under the [MIT License](LICENSE).
