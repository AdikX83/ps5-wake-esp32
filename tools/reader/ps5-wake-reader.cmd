<# : PS5 Wake - DualSense address reader for Windows (batch + PowerShell in one file)
@echo off
setlocal
title PS5 Wake - DualSense address reader
powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-Expression ([System.IO.File]::ReadAllText('%~f0'))"
echo.
pause
exit /b
#>
# Reads the Bluetooth addresses of the DualSense and of the PS5 it is paired with, over USB
# (HID feature report 0x09, read only). Nothing is installed or changed on the controller.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32.SafeHandles;

public static class DualSenseReader {
    [DllImport("hid.dll")] static extern void HidD_GetHidGuid(out Guid guid);
    [DllImport("hid.dll", SetLastError = true)] static extern bool HidD_GetAttributes(SafeFileHandle h, ref HIDD_ATTRIBUTES a);
    [DllImport("hid.dll", SetLastError = true)] static extern bool HidD_GetFeature(SafeFileHandle h, byte[] buffer, int length);
    [DllImport("setupapi.dll", SetLastError = true)] static extern IntPtr SetupDiGetClassDevs(ref Guid g, IntPtr e, IntPtr p, int flags);
    [DllImport("setupapi.dll", SetLastError = true)] static extern bool SetupDiEnumDeviceInterfaces(IntPtr set, IntPtr info, ref Guid g, int index, ref SP_DEVICE_INTERFACE_DATA data);
    [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Unicode)] static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr set, ref SP_DEVICE_INTERFACE_DATA data, IntPtr detail, int size, out int required, IntPtr info);
    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)] static extern SafeFileHandle CreateFile(string name, uint access, uint share, IntPtr security, uint creation, uint flags, IntPtr template);

    [StructLayout(LayoutKind.Sequential)] struct SP_DEVICE_INTERFACE_DATA { public int cbSize; public Guid InterfaceClassGuid; public int Flags; public IntPtr Reserved; }
    [StructLayout(LayoutKind.Sequential)] struct HIDD_ATTRIBUTES { public int Size; public ushort VendorID; public ushort ProductID; public ushort VersionNumber; }

    // Returns { PS5 address, controller address } or null when no USB DualSense answers.
    public static string[] Read() {
        Guid guid;
        HidD_GetHidGuid(out guid);
        IntPtr set = SetupDiGetClassDevs(ref guid, IntPtr.Zero, IntPtr.Zero, 0x12); // PRESENT | DEVICEINTERFACE
        List<string> paths = new List<string>();
        try {
            for (int i = 0; ; i++) {
                SP_DEVICE_INTERFACE_DATA data = new SP_DEVICE_INTERFACE_DATA();
                data.cbSize = Marshal.SizeOf(data);
                if (!SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref guid, i, ref data)) break;
                int required;
                SetupDiGetDeviceInterfaceDetail(set, ref data, IntPtr.Zero, 0, out required, IntPtr.Zero);
                IntPtr detail = Marshal.AllocHGlobal(required);
                try {
                    Marshal.WriteInt32(detail, IntPtr.Size == 8 ? 8 : 6);
                    if (SetupDiGetDeviceInterfaceDetail(set, ref data, detail, required, out required, IntPtr.Zero))
                        paths.Add(Marshal.PtrToStringUni(new IntPtr(detail.ToInt64() + 4)));
                } finally { Marshal.FreeHGlobal(detail); }
            }
        } finally { SetupDiDestroyDeviceInfoList(set); }
        foreach (string path in paths) {
            SafeFileHandle handle = Open(path);
            if (handle == null) continue;
            using (handle) {
                HIDD_ATTRIBUTES attributes = new HIDD_ATTRIBUTES();
                attributes.Size = Marshal.SizeOf(attributes);
                if (!HidD_GetAttributes(handle, ref attributes) || attributes.VendorID != 0x054C ||
                    (attributes.ProductID != 0x0CE6 && attributes.ProductID != 0x0DF2)) continue;
                byte[] report = new byte[20];
                report[0] = 0x09; // pairing information
                if (!HidD_GetFeature(handle, report, report.Length)) continue;
                return new string[] { Mac(report, 10), Mac(report, 1) };
            }
        }
        return null;
    }

    static SafeFileHandle Open(string path) {
        foreach (uint access in new uint[] { 0xC0000000u, 0u }) {
            SafeFileHandle h = CreateFile(path, access, 3, IntPtr.Zero, 3, 0, IntPtr.Zero);
            if (!h.IsInvalid) return h;
            h.Dispose();
        }
        return null;
    }

    static string Mac(byte[] b, int offset) {
        StringBuilder text = new StringBuilder();
        for (int i = 5; i >= 0; i--) {
            text.Append(b[offset + i].ToString("X2"));
            if (i > 0) text.Append(':');
        }
        return text.ToString();
    }
}
'@

Write-Host ''
Write-Host 'PS5 Wake - DualSense address reader' -ForegroundColor Cyan
Write-Host 'Connect the DualSense (already paired with your PS5) to this computer with a USB cable.'
Write-Host ''
$result = $null
for ($i = 0; $i -lt 60 -and -not $result; $i++) {
    $result = [DualSenseReader]::Read()
    if (-not $result) {
        if ($i -eq 0) { Write-Host 'Waiting for the controller (up to 60 s)...' }
        Start-Sleep -Seconds 1
    }
}
if (-not $result) {
    Write-Host 'No DualSense found over USB. Check the cable (it must carry data, not only power) and try again.' -ForegroundColor Red
    exit 1
}
if ($result[0] -eq $result[1] -or $result[0] -eq '00:00:00:00:00:00') {
    Write-Host 'The controller is not paired with a console. Pair it with your PS5 first.' -ForegroundColor Red
    exit 1
}
$code = "PS5WAKE:$($result[0]),$($result[1])"
Write-Host ('PS5 Bluetooth address:       ' + $result[0])
Write-Host ('Controller Bluetooth address: ' + $result[1])
Write-Host ''
Write-Host 'Setup code:' -NoNewline
Write-Host (' ' + $code) -ForegroundColor Green
try {
    Set-Clipboard -Value $code
    Write-Host 'The setup code has been copied to the clipboard - paste it into the PS5 Wake panel.'
} catch {
    Write-Host 'Copy the setup code above and paste it into the PS5 Wake panel.'
}
